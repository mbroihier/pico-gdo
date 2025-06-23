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
 
/*
 * modified spp_counter.c
 * 
 */

#define BTSTACK_FILE__ "spp_counter.c"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

#include "btstack.h"
#include "lock.h"

#define RFCOMM_SERVER_CHANNEL 1
#define HEARTBEAT_PERIOD_MS 50

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void rfcomm_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static int toggle = true;

static bd_addr_t rfcomm_addr;
static uint16_t rfcomm_channel_id;
static uint8_t  spp_service_buffer[150];
static uint8_t btstack_state = 0; // stack down
static btstack_packet_callback_registration_t hci_event_callback_registration;

static void spp_service_setup(void){

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    l2cap_init();

    // init SDP, create record for SPP and register with SDP
    sdp_init();
    memset(spp_service_buffer, 0, sizeof(spp_service_buffer));
    spp_create_sdp_record(spp_service_buffer, 0x10001, RFCOMM_SERVER_CHANNEL, "rfcomm");
    sdp_register_service(spp_service_buffer);
    printf("SDP service record size: %u\n", de_get_len(spp_service_buffer));
}

static btstack_timer_source_t heartbeat;
static char lineBuffer[30];
static int messageLength = 0;
static void  heartbeat_handler(struct btstack_timer_source *ts){
    static int counter = 0;

    if (rfcomm_channel_id){
      char * ref = 0;
      int parms[] = {1807, 45289, 214326};
      lock lock_object(parms);
      uint64_t rn = time_us_64();
      uint8_t seed[] = {(rn >> 24) & 0xff, (rn >> 16) & 0xff, (rn >> 8) & 0xff, rn & 0xff};
      lock_object.make((char *)seed);
      messageLength = lock_object.getARealKey(&ref);
      printf("sending %d bytes:\n", messageLength);
      for (int i = 0; i < messageLength; i++) {
	lineBuffer[i] = *ref++;
	printf("%2.2x ", lineBuffer[i]);
      }
      printf("\n");
      rfcomm_request_can_send_now_event(rfcomm_channel_id);
    }

    btstack_run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
} 

static void one_shot_timer_setup(void){
    // set one-shot timer
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(&heartbeat);
}


static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
  UNUSED(channel);
  bd_addr_t event_addr;
  uint8_t   rfcomm_channel_nr;
  uint16_t  mtu;
  int i;

  switch (packet_type) {
  case HCI_EVENT_PACKET:
    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE: {
      uint8_t state = btstack_event_state_get_state(packet);
      printf("Got a BTstack event state change: %x\n", state);
      if (state == HCI_STATE_WORKING) {
	printf("BTstack is up and running\n");
	btstack_state = 1;  // stack is up
      }
      break;
    }
    case HCI_EVENT_PIN_CODE_REQUEST:
      // inform about pin code request
      printf("HCI EVENT Pin code request - using '0000'\n");
      hci_event_pin_code_request_get_bd_addr(packet, event_addr);
      gap_pin_code_response(event_addr, "0000");
      break;

    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
      // ssp: inform about user confirmation request
      //printf("HCI EVENT - SSP User Confirmation Request with numeric value '%06"PRIu32"'\n", little_endian_read_32(packet, 8));
      printf("HCI EVENT - SSP User Confirmation Auto accept\n");
      break;

    default:
      break;
    }
    break;

  default:
    break;
  }

}
static void rfcomm_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
  UNUSED(channel);
  bd_addr_t event_addr;
  uint8_t   rfcomm_channel_nr;
  uint16_t  mtu;
  int i;
  printf("RFCOMM PACKET HANDLER\n");

  switch (packet_type) {
  case HCI_EVENT_PACKET:
    switch (hci_event_packet_get_type(packet)) {
    case RFCOMM_EVENT_INCOMING_CONNECTION:
      rfcomm_event_incoming_connection_get_bd_addr(packet, event_addr);
      rfcomm_channel_nr = rfcomm_event_incoming_connection_get_server_channel(packet);
      rfcomm_channel_id = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);
      printf("RFCOMM channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
      rfcomm_accept_connection(rfcomm_channel_id);
      break;
               
    case RFCOMM_EVENT_CHANNEL_OPENED:
      if (rfcomm_event_channel_opened_get_status(packet)) {
	printf("RFCOMM channel open failed, status %u\n", rfcomm_event_channel_opened_get_status(packet));
	rfcomm_channel_id = 0;
      } else {
	rfcomm_channel_id = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
	mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
	printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_channel_id, mtu);
      }
      break;
    case RFCOMM_EVENT_CAN_SEND_NOW:
      printf("RFCOMM can send data data\n");
      rfcomm_send(rfcomm_channel_id, (uint8_t*) lineBuffer, (uint16_t) messageLength);
      break;
                
    case RFCOMM_EVENT_CHANNEL_CLOSED:
      printf("RFCOMM channel closed\n");
      rfcomm_channel_id = 0;
      break;
    case RFCOMM_EVENT_REMOTE_MODEM_STATUS:
      printf("RFCOMM remote modem status: %x ea,fc,rtc,rtr,ic,dv\n", packet[4]);
      break;
    case RFCOMM_EVENT_PORT_CONFIGURATION:
      printf("RFCOMM port configuration change\n");
      break;
    default:
      printf("rfcomm default packet - type was: %2.2x\n", hci_event_packet_get_type(packet));
      for (uint i = 0; i < 30 ; i++) {
	printf("%2.2x ", packet[i]);
      }
      printf("\n"); 
      break;
    }
    break;

  case RFCOMM_DATA_PACKET:
    printf("RFCOM_DATA_PACKET - type was: %2.2x, size was: %d\n", hci_event_packet_get_type(packet), size);
    printf("%s\n", packet);
    break;

  default:
    break;
  }

}

int btstack_main(int argc, const char * argv[]);
int btstack_main(int argc, const char * argv[]){
    (void)argc;
    (void)argv;

    one_shot_timer_setup();
    sleep_ms(1000);
    spp_service_setup();
    //hci_dump_init(hci_dump_embedded_stdout_get_instance());

    gap_discoverable_control(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_set_local_name("pico-gdo");
    hci_power_control(HCI_POWER_ON);
    while (btstack_state == 0){
      sleep_ms(100);
    }
    sscanf_bd_addr("00:21:E9:D6:77:BA", rfcomm_addr); // iMac
    //sscanf_bd_addr("B8:27:EB:69:B1:42", rfcomm_addr); // rpi3

    printf("setting up rfcomm\n");
    rfcomm_init();
    rfcomm_register_service(packet_handler, RFCOMM_SERVER_CHANNEL, 0xffff);  // reserved channel, mtu limited by l2cap
    rfcomm_create_channel(rfcomm_packet_handler, rfcomm_addr, 1, &rfcomm_channel_id);
    rfcomm_request_can_send_now_event(rfcomm_channel_id);
    printf("rfcomm_channel_id %4x\n", rfcomm_channel_id);

    return 0;
}

