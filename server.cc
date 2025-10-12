/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */

#define BTSTACK_FILE__ "server.cc"

// *****************************************************************************
/* EXAMPLE_START(RFCOMM server): Dual Mode - SPP
 * 
 */
// *****************************************************************************
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "hardware/gpio.h"
#include "btstack.h"
#include "lock.h"


#define PIN 15
#define RFCOMM_SERVER_CHANNEL 1
#define HEARTBEAT_PERIOD_MS 10

static uint16_t  rfcomm_channel_id;
static uint8_t   spp_service_buffer[150];
static int       le_notification_enabled;
static hci_con_handle_t att_con_handle;

// THE Couner
static btstack_timer_source_t heartbeat;
static int  counter = 0;
static char counter_string[30];
static int  counter_string_len;

static btstack_packet_callback_registration_t hci_event_callback_registration;

static uint8_t gatt_service_buffer[70];

static std::map<int, int>  keys;
static std::map<int, int>  keys_index;
static int age = 0;
/*
 * @section Advertisements 
 *
 * @text The Flags attribute in the Advertisement Data indicates if a device is dual-mode or le-only.
 */
/* LISTING_START(advertisements): Advertisement data: Flag 0x02 indicates dual-mode device */
const uint8_t adv_data[] = {
    // Flags general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x02,
    // Name
    0x0b, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'L', 'E', ' ', 'C', 'o', 'u', 'n', 't', 'e', 'r', 
    // Incomplete List of 16-bit Service Class UUIDs -- FF10 - only valid for testing!
    0x03, BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x10, 0xff,
};
/* LISTING_END */
uint8_t adv_data_len = sizeof(adv_data);


/* 
 * @section Packet Handler
 * 
 * @text The packet handler of the combined example is just the combination of the individual packet handlers.
 */
static bool is_still_locked = true;
static uint16_t rfcomm_con_handle;
static uint32_t connection_count = 0;
static uint32_t reply_count = 0;
static bool ready_to_reply = false;
lock * lock_ptr;
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
  UNUSED(channel);

  bd_addr_t event_addr;
  uint8_t   rfcomm_channel_nr;
  uint16_t  mtu;
  int i;

  switch (packet_type) {
  case HCI_EVENT_PACKET:
    switch (hci_event_packet_get_type(packet)) {
    case HCI_EVENT_PIN_CODE_REQUEST:
      // inform about pin code request
      printf("Pin code request - using '0000'\n");
      hci_event_pin_code_request_get_bd_addr(packet, event_addr);
      gap_pin_code_response(event_addr, "0000");
      break;

    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
      // inform about user confirmation request
      printf("SSP User Confirmation Request with numeric value '%06" PRIu32 "'\n", little_endian_read_32(packet, 8));
      printf("SSP User Confirmation Auto accept\n");
      break;

    case HCI_EVENT_DISCONNECTION_COMPLETE:
      le_notification_enabled = 0;
      break;

    case ATT_EVENT_CAN_SEND_NOW:
      printf("ERROR this should never happen!!!!!\n");
      //att_server_notify(att_con_handle, ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, (uint8_t*) counter_string, counter_string_len);
      break;

    case RFCOMM_EVENT_INCOMING_CONNECTION:
      // data: event (8), len(8), address(48), channel (8), rfcomm_cid (16)
      rfcomm_event_incoming_connection_get_bd_addr(packet, event_addr); 
      rfcomm_con_handle = rfcomm_event_incoming_connection_get_con_handle(packet);
      rfcomm_channel_nr = rfcomm_event_incoming_connection_get_server_channel(packet);
      rfcomm_channel_id = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);
      printf("RFCOMM channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
      if (strncmp(bd_addr_to_str(event_addr), "28:CD:C1:13:15:2E", sizeof("28:CD:C1:13:15:2E")) == 0) {
        // only accept connections from the expected client
        rfcomm_accept_connection(rfcomm_channel_id);
        connection_count++;
      }
      break;
					
    case RFCOMM_EVENT_CHANNEL_OPENED:
      // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
      if (rfcomm_event_channel_opened_get_status(packet)) {
        printf("RFCOMM channel open failed, status 0x%02x\n", rfcomm_event_channel_opened_get_status(packet));
      } else {
        rfcomm_channel_id = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
        mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
        printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_channel_id, mtu);
      }
      break;

    case RFCOMM_EVENT_CAN_SEND_NOW: {
      char status_string[128];
      int string_len = 0;
      if (reply_count < connection_count && ready_to_reply) {
        if (is_still_locked) {
          printf("Sending back failed\n");
          string_len = sprintf(status_string, "Command Failed\n");
        } else {
          printf("Sending back success\n");
          string_len = sprintf(status_string, "Command Successful\n");
        }
        ready_to_reply = false;
        reply_count++;
        rfcomm_send(rfcomm_channel_id, (uint8_t*) status_string, string_len);
      }
      break;
    }
    case RFCOMM_EVENT_CHANNEL_CLOSED:
      printf("RFCOMM channel closed\n");
      rfcomm_channel_id = 0;
      printf("releasing button\n");
      gpio_put(PIN, 0);
      break;
                
    default:
      break;
    }
    break;
                        
  case RFCOMM_DATA_PACKET: {
    printf("RFCOM_DATA_PACKET - type was: %2.2x, size was: %d\n", hci_event_packet_get_type(packet), size);
    if (lock_ptr->check_another_key((char *)packet)) {
      printf("Received a INVALID key\n");
      is_still_locked = true;
    } else {
      printf("Received a numerically valid key\n");
      for (int i = 0; i < size ; i++) {
        printf("%2.2x ", packet[i]);
      }
      printf("\n");
      int seed = ((packet[0] << 24) + ((packet[1] << 16) & 0xff0000) +
                  ((packet[2] << 8) & 0xff00) + (packet[3] & 0xff));
      std::map<int, int>::iterator location = keys.find(seed);
      if (location == keys.end()) {
        keys[seed] = 1;
        if (age > 1000) age = 0;
        if (keys.size() >= 1000) {
          keys.erase(keys_index[age]);
        }
        keys_index[age++] = seed;
        if (keys.size() == 1) {
          is_still_locked = true;
        } else {
          printf("pushing button\n");
          gpio_put(PIN, 1);
          is_still_locked = false;
        }
      } else {
        printf("duplicate key, don't unlock\n");
        is_still_locked = true;
      }
    }
    ready_to_reply = true;
    rfcomm_request_can_send_now_event(rfcomm_channel_id);
    break;
  }
  default:
    break;
  }
}

/*
 * @section Heartbeat Handler
 * 
 * @text Similar to the packet handler, the heartbeat handler is the combination of the individual ones.
 * After updating the counter, it requests an ATT_EVENT_CAN_SEND_NOW and/or RFCOMM_EVENT_CAN_SEND_NOW
 */

 /* LISTING_START(heartbeat): Combined Heartbeat handler */
static void heartbeat_handler(struct btstack_timer_source *ts){
  btstack_run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS);
  btstack_run_loop_add_timer(ts);
} 
/* LISTING_END */

/*
 * @section Main Application Setup
 *
 * @text As with the packet and the heartbeat handlers, the combined app setup contains the code from the individual example setups.
 */

/* LISTING_START(MainConfiguration): Init L2CAP RFCOMM SDO SM ATT Server and start heartbeat timer */
int btstack_main(void);
int btstack_main(void)
{
  uint8_t * profile_data;
  l2cap_init();
  gpio_init(PIN);
  gpio_set_dir(PIN, GPIO_OUT);
  rfcomm_init();
  rfcomm_register_service(packet_handler, RFCOMM_SERVER_CHANNEL, 0xffff);

  // init SDP, create record for SPP and register with SDP
  sdp_init();
  memset(spp_service_buffer, 0, sizeof(spp_service_buffer));
  spp_create_sdp_record(spp_service_buffer, sdp_create_service_record_handle(), RFCOMM_SERVER_CHANNEL, "GDO Server");
  btstack_assert(de_get_len( spp_service_buffer) <= sizeof(spp_service_buffer));
  sdp_register_service(spp_service_buffer);

  gap_set_local_name("GDO Server 00:00:00:00:00:00");
  gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
  gap_discoverable_control(1);

  // setup SM: Display only
  sm_init();

  // register for HCI events
  hci_event_callback_registration.callback = &packet_handler;
  hci_add_event_handler(&hci_event_callback_registration);

  // setup advertisements
  uint16_t adv_int_min = 0x0030;
  uint16_t adv_int_max = 0x0030;
  uint8_t adv_type = 0;

  // set one-shot timer
  heartbeat.process = &heartbeat_handler;
  btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
  btstack_run_loop_add_timer(&heartbeat);

  // turn on!
  hci_power_control(HCI_POWER_ON);
	    
  return 0;
}
/* LISTING_END */
/* EXAMPLE_END */
