#include "vivacity/core/detector_tracker_frame.pb.h"
#include <netinet/in.h>
#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define UDP_LISTENER_PORT 8000

// This depends on how many sensors are sending here, and how many zones per sensor - it can vary a lot
#define MAX_EXPECTED_ZONES 32

// This is a typical ethernet MTU, but most DetectorTrackerFrame protobuf UDP packets will only be ~100 bytes
#define RECV_BUFFER_SIZE 1500


typedef struct ZoneOrientedOccupancy {
  uint32_t zone_id;

  uint32_t total_occupancy;
  uint32_t occupancy_by_class[_vivacity_core_ClassifyingDetectorClassTypes_ARRAYSIZE];

  uint32_t total_stopped_vehicles;
  uint32_t stopped_vehicles_by_class[_vivacity_core_ClassifyingDetectorClassTypes_ARRAYSIZE];
} _ZoneOrientedOccupancy;

typedef struct AllZones {
  bool state_changed;
  struct ZoneOrientedOccupancy zones[MAX_EXPECTED_ZONES];
} _AllZones;

typedef _AllZones AllZones;

bool has_zone_state_changed(vivacity_core_ZonalFeatures *received_zonal_features, AllZones *existing_zone_state, int current_zone) {

  for(int class_id = 0; class_id < _vivacity_core_ClassifyingDetectorClassTypes_ARRAYSIZE; class_id++) {
    bool class_not_included_in_received = false;

    // Since class_features are only included when greater than zero, any that are in this message must be > 0 and should be compared, and any others are implied to be 0
    for(int j = 0; j < received_zonal_features->class_features_count; j++) {
      vivacity_core_ClassifyingDetectorClassTypes recvd_class = received_zonal_features->class_features[j].class_type;

      if (class_id == received_zonal_features->class_features[j].class_type) {
        if (existing_zone_state->zones[current_zone].occupancy_by_class[recvd_class] != received_zonal_features->class_features[j].occupancy) {
          return true;
        }

        if (existing_zone_state->zones[current_zone].stopped_vehicles_by_class[recvd_class] != received_zonal_features->class_features[j].stopped_vehicles_count) {
          return true;
        }
        break;
      } else if (j == received_zonal_features->class_features_count - 1){
        // If we reached the last class_features entry and this class_id still wasn't found, then it wasn't included in the received class_features
        class_not_included_in_received = true;
      }
    }

    // This class can be assumed to be received as zero, so check whether it is already zero in the state
    if (class_not_included_in_received) {
      if (existing_zone_state->zones[current_zone].occupancy_by_class[class_id] != 0) {
        return true;
      }
      if (existing_zone_state->zones[current_zone].stopped_vehicles_by_class[class_id] != 0) {
        return true;
      }
    }
  }

  // Check whether the existing_zone_state totals for this zone are going to change from this message
  if (existing_zone_state->zones[current_zone].total_occupancy != received_zonal_features->aggregated_occupancy ||
      existing_zone_state->zones[current_zone].total_stopped_vehicles != received_zonal_features->aggregated_stopped_vehicles_count) {
    return true;
  }

  return false;
}


// This callback will be called once for each `repeated ZonalFeatures` in DetectorTrackerFrame.zone_oriented_features
bool read_zonal_features_protobuf(pb_istream_t *stream, const pb_field_iter_t *field, void **arg){
  while (stream->bytes_left)
  {
    // The arg is a reference to the global store of zone state
    AllZones *existing_zone_state = ((AllZones*) *arg);

    // Allocate a struct for to parse the current zone's ZonalFeatures into
    vivacity_core_ZonalFeatures received_zonal_features = vivacity_core_ZonalFeatures_init_zero;

    if (!pb_decode(stream, vivacity_core_ZonalFeatures_fields, &received_zonal_features)){
      return false;
    }

    // Loop through the array of AllZones until we find the Zone ID for the zone we've just decoded
    for (int i = 0; i < MAX_EXPECTED_ZONES; i++) {

      // If the stored zone_id matches, then store the total occupancies in the struct at this index
      // or if we've reached an index which has zone_id 0, then we didn't have the zone_id we received stored already, so we can store it at this index
      if (existing_zone_state->zones[i].zone_id == received_zonal_features.zone_id || existing_zone_state->zones[i].zone_id == 0) {

        // Check whether the zonal features we've received result in a change of state - this isn't necessary but might be useful to rate limit output
        existing_zone_state->state_changed = has_zone_state_changed(&received_zonal_features, existing_zone_state, i);

        if (existing_zone_state->zones[i].zone_id == 0) {
          // We reached an index in our existing_zone_state with zone_id == 0, meaning we haven't seen this zone_id before, so we can store it at this index
          existing_zone_state->zones[i].zone_id = received_zonal_features.zone_id;
          existing_zone_state->state_changed = true;
        }

        // Reset any existing stored class occupancies to zero, since class zonal features will only be populated in the message if they're greater than zero, so we assume they're zero and populate them if we find them
        for(int j = 0; j < _vivacity_core_ClassifyingDetectorClassTypes_ARRAYSIZE; j++) {
          // Index the class-based features by all the values in the ClassifyingDetectorClassTypes enum
          existing_zone_state->zones[i].occupancy_by_class[j] = 0;
          existing_zone_state->zones[i].stopped_vehicles_by_class[j] = 0;
        }

        // Store the total occupancy for this zone at this index
        existing_zone_state->zones[i].total_occupancy = received_zonal_features.aggregated_occupancy;
        existing_zone_state->zones[i].total_stopped_vehicles = received_zonal_features.aggregated_stopped_vehicles_count;

        // Store the occupancies for whichever class_features were sent, indexed by class_type
        for(int j = 0; j < received_zonal_features.class_features_count; j++) {
          vivacity_core_ClassifyingDetectorClassTypes recvd_class = received_zonal_features.class_features[j].class_type;
          existing_zone_state->zones[i].occupancy_by_class[recvd_class] = received_zonal_features.class_features[j].occupancy;
          existing_zone_state->zones[i].stopped_vehicles_by_class[recvd_class] = received_zonal_features.class_features[j].stopped_vehicles_count;
        }
        break;
      }
    }

  }
  return true;
}

void print_occupancy_table(vivacity_core_DetectorTrackerFrame *message, AllZones *zone_data) {
  printf("Zone ID  | %5s", "TOTAL");

  // Print the class names as a table header
  for(int i = 1; i < _vivacity_core_ClassifyingDetectorClassTypes_ARRAYSIZE; i++) {
    printf("%14s", vivacity_core_ClassifyingDetectorClassTypes_name(i));
  }

  printf("\n");

  // Each zone will be a row
  for (int j = 0; j < MAX_EXPECTED_ZONES; j++) {

    // Stop once we get to an index with zone_id ==0, since this will be empty
    if (zone_data->zones[j].zone_id == 0) {
      break;
    }

    // Print the Zone ID and total occupancies in the first 2 columns
    printf("%3d      | %5d", zone_data->zones[j].zone_id, zone_data->zones[j].total_occupancy);

    // Loop through each class and print its occupancy for this zone
    for(int i = 1; i < _vivacity_core_ClassifyingDetectorClassTypes_ARRAYSIZE; i++) {
      printf("%14d", zone_data->zones[j].occupancy_by_class[i]);
    }
    // End the row
    printf("\n");
  }

  // End the table
  printf("\n\n");
}

int listen_udp() {
  int sock;
  struct sockaddr_in socket_addr;

  // Create a IP datagram (UDP) network socket
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)   {
    perror("Couldn't open datagram socket");
    exit(1);
  }

  // Zero out the sockaddr_in struct
  bzero((char *) &socket_addr, sizeof(socket_addr));

  // Configure to listen for IP traffic on all interfaces on the specified port
  socket_addr.sin_family = AF_INET;
  socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  socket_addr.sin_port = htons(UDP_LISTENER_PORT);

  // Bind the socket to the configured address to start listening
  // Unread packets will be buffered to an extent by the Linux kernel before being dropped (up to 278 packets, or 50kB, depending on sysctl config)
  if (bind(sock, (struct sockaddr *) &socket_addr, sizeof(socket_addr))) {
    perror("Couldn't bind datagram socket");
    exit(1);
  }

  printf("Listening for UDP packets on port %d\n", ntohs(socket_addr.sin_port));

  return sock;
}

int main() {
  bool decode_status;

  // Allocate a structure to store the current occupancies etc. for all zones
  AllZones zone_state = {0};

  // Create and bind a UDP socket to start listening for packets
  int sock = listen_udp();

  int bytes_received;
  unsigned char recv_buffer[RECV_BUFFER_SIZE];

  while ((bytes_received = read(sock, recv_buffer, RECV_BUFFER_SIZE)) > 0) {

    // Allocate space for the decoded DetectorTrackerFrame message.
    vivacity_core_DetectorTrackerFrame message = vivacity_core_DetectorTrackerFrame_init_zero;

    // Pass a pointer to the structure we're using to store the occupancies in
    // This will be accessible in our decode callback read_zonal_features_protobuf(), so we can decide where to put our data
    message.zone_oriented_features.arg = &zone_state;

    // Since the zone_oriented_features field is `repeated`, we need to set a callback to decode each instance - This avoids uncontrolled allocation during decoding.
    // This callback approach is used where an unknown number of objects might be in a repeated field, and you don't want to allocate for a maximum
    // If you want to statically allocate the array size to avoid using callbacks, you can add  [(nanopb).max_count = 25] to the `zone_oriented_features` field definition before the semicolon in the .proto file
    // (for an example, see zonal_features.proto -> ZonalFeatures.class_features)

    message.zone_oriented_features.funcs.decode = read_zonal_features_protobuf;

    // Create a nanopb input stream that reads from the buffer
    pb_istream_t stream = pb_istream_from_buffer(recv_buffer, bytes_received);

    // Decode the message into the struct we allocated
    decode_status = pb_decode(&stream, vivacity_core_DetectorTrackerFrame_fields, &message);

    // Check for errors
    if (!decode_status)
    {
      printf("Decoding protobuf failed: %s\n", PB_GET_ERROR(&stream));
      return 1;
    }

    printf("Sensor ID: %d    Timestamp: %lu\n", message.vision_program_id,  message.frame_time_microseconds);
    if (zone_state.state_changed) {
      // Pretty print our current occupancy data
      print_occupancy_table(&message, &zone_state);
    }
  }

  close(sock);
  return 0;
}
