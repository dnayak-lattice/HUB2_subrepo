/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef GARD_HUB_IFACE_H
#define GARD_HUB_IFACE_H

/**
 * Make sure the compiler search path uses the types.h in the interface folder
 * for maximum compatibility between GARD and HUB.
 */
#include "types.h"

/**
 * This is the DISCOVERY CMD RESPONSE SIGNATURE for pre-version 1 which helps
 * identify the GARD code that does not support returning
 * FIRMWARE VERSION NUMBER command.
 */
#define HUB_GARD_DISCOVER_SIGNATURE_PRE_VER_1 "I AM GARD"

#define USE_GARD_HUB_IFACE_VER_3

#ifdef USE_GARD_HUB_IFACE_VER_3
#define GARD_HUB_IFACE_VER 3
#else
/* Default to VER_1. We have moved ahead from Pre-Ver-1. */
#define GARD_HUB_IFACE_VER 1
#endif

/* HUB GARD DISCOVERY CMD RESPONSE SIGNATURE */
#define HUB_GARD_DISCOVER_SIGNATURE "I M GARD" STR(GARD_HUB_IFACE_VER)

enum special_markers {
	/**
	 * The following markers are used to indicate the start and end of data
	 * transfer over UART. These markers are used to identify the boundaries
	 * of the data being transferred and to ensure that the data is received
	 * correctly.
	 */
	END_OF_DATA_MARKER   = 0xE0DBE0DBU,
	START_OF_DATA_MARKER = 0x50DB50DBU,
	ACK_BYTE             = 0xAC,
	NAK_BYTE             = 0xCA,
	SEND_DATA            = 0x5D,
};

/**
 * The following are the command IDs that are placed in the command_id field
 * of the host_requests structure. These IDs are used to identify
 * the command being sent from the host to the GARD over UART.
 * Each command ID corresponds to a specific command that the GARD can execute.
 * The command IDs are used to determine the command body structure and the
 * expected response structure.
 */
enum host_request_command_ids {
	GARD_DISCOVERY                     = 0x00u,
	SEND_DATA_TO_GARD_FOR_OFFSET       = 0x01u,
	RECV_DATA_FROM_GARD_AT_OFFSET      = 0x02u,
	READ_REG_VALUE_FROM_GARD_AT_OFFSET = 0x03u,
	WRITE_REG_VALUE_TO_GARD_AT_OFFSET  = 0x04u,
	GET_ML_ENGINE_STATUS               = 0x05u,
	GARD_APP_LAYER_SEND                = 0x07u,
	HUB_APP_LAYER_REQUEST              = 0x08u,
	HUB_APP_LAYER_SEND                 = 0x09u,
	COORDINATE_DEFAULT_ORIGINATOR      = 0x0Au,
	GARD_APP_LAYER_REQUEST             = 0x0Bu,
	ORIGINATOR_SYNC                    = 0x0Cu,
	PAUSE_PIPELINE                     = 0x20u,
	CAPTURE_RESCALED_IMAGE             = 0x21u,
	RESUME_PIPELINE                    = 0x22u,
	UPGRADE_FIRMWARE                   = 0x23u,
	GET_FIRMWARE_VERSION               = 0x24u,
	GET_SUPPORTED_CMNDS_LIST           = 0x25u,
	GET_SUPPORTED_SUB_CMNDS_LIST       = 0x26u,
};

/**
 * The following are the control codes that are used in the command body of
 * the host_requests structure. These control codes are used to
 * specify additional options for the command being sent to the GARD.
 * Each control code is a bit flag that can be combined with others to specify
 * multiple options.
 */
enum control_codes {
	/**
	 * When CC_MTU_SIZE is set, the mtu_size field in the command body
	 * specifies the maximum size of the data transfer. If the data to be sent
	 * exceeds this size, it will be split into multiple packets of size
	 * mtu_size. This also implies that each packet will contain a header
	 * containing a packet number and the CRC calculated over the data contained
	 * within that packet.
	 * Sender:
	 * The sender will wait for an ACK from the receiver for the previous packet
	 * before sending the next packet. If ACK is not received within a certain
	 * timeout, or if a NAK is received then the sender will re-send the packet
	 * to the receiver.
	 * Receiver:
	 * The receiver will validate the CRC of each packet received and
	 * will send an ACK if the packet is received successfully. If the packet
	 * contents fail the CRC validation then the receiver will send a NAK to the
	 * sender to indicate that the packet needs to be re-sent. The receiver will
	 * maintain a running count of the received packets and ensure that the next
	 * packet received is the next in sequence. If a packet is received out of
	 * order then the receiver will send a NAK with the expected packet number
	 * embedded in the NAK packet.
	 * ***** Currently this is not implemented in the GARD firmware. *****
	 */
	CC_USE_MTU_SIZE        = (1 << 0),

	/**
	 * An optional ACK can be sent by the receiver after the data transfer
	 * is complete by setting the bit CC_SEND_ACK_AFTER_XFER in the control
	 * field. This is useful for the sender to know that the receiver has
	 * received the data successfully. This field does not make sense if
	 * CC_USE_MTU_SIZE is set as CC_USE_MTU_SIZE setting will do a more rigorous
	 * validation of the data transfer.
	 */
	CC_SEND_ACK_AFTER_XFER = (1 << 1),

	/**
	 * When CC_CHECKSUM_PRESENT is set then a single 32-bit checksum follows
	 * the data transfer. This field is redundant if CC_USE_MTU_SIZE field is
	 * set.
	 */
	CC_CHECKSUM_PRESENT    = (1 << 2),

	/**
	 * The CC_APP_DATA bit indicates that during the execution of
	 * RECV_DATA_FROM_GARD_AT_OFFSET command, the data to be sent to the Host is
	 * generated by the App Module and it should be read from the buffer pointed
	 * by app_tx_buffer variable. The offset_address field in the command body
	 * should be ignored.
	 */
	CC_APP_DATA            = (1 << 3),
};

/**
 * The following enum captures all the possible image formats. The binary image
 * data captured from the camera and sent to HUB is crafted in one of these
 * formats.
 */
enum image_formats {
	/* This is invalid and should not be used. */
	IMAGE_FORMAT__INVALID        = 0x0u,

	/**
	 * The RGB values in the image are in non-planar fashion. In other words
	 * each pixel is represented by 3 consecutive bytes representing R, G and B
	 * values.
	 */
	IMAGE_FORMAT__RGB_NON_PLANAR = 0x1u,

	/**
	 * The RGB values in the image are in planar fashion. In other words the
	 * first 1/3rd of the buffer is the R values, the next 1/3rd is the G values
	 * and the last 1/3rd is the B values.
	 */
	IMAGE_FORMAT__RGB_PLANAR     = 0x2u,

	/**
	 * The image is a grayscale image. The image is represented by a single byte
	 * for each pixel.
	 */
	IMAGE_FORMAT__GRAYSCALE      = 0x3u,
};

enum firmware_upgrade_sub_command_ids {
	/**
	 * Invalid comand. We mark '0' as not a valid value.
	 */
	USCID__INVALID_SUB_COMMAND     = 0x0u,

	/**
	 * Return information about the resources to be used within GARD during
	 * xfer of firmware binary.
	 */
	USCID__GET_XFER_INFORMATION    = 0x1u,

	/**
	 * Write the firmware binary (complete or partial) within GARD from volatile
	 * to persistent storage.
	 */
	USCID__WRITE_FIRMWARE_TO_FLASH = 0x2u,

	/**
	 * Check if operation is complete.
	 */
	USCID__IS_OPERATION_COMPLETE   = 0x3u,
};

enum app_module_ids {
	/**
	 * Module ID is not provided. In this case the rest of the version values
	 * are of the Platform firmware.
	 */
	AMI__INVALID = 0x00u,

	/**
	 * Version information is of HMI module.
	 */
	AMI__HMI     = 0x01u,

	/**
	 * Version information is of MOD module.
	 */
	AMI__MOD     = 0x02u,

	/**
	 * Version information is of DD module.
	 */
	AMI__DD      = 0x03u,
};

enum default_originators {
	DEFAULT_ORIGINATOR__INVALID     = 0x00u,
	DEFAULT_ORIGINATOR__HUB         = 0x01u,
	DEFAULT_ORIGINATOR__GARD        = 0x02u,
	DEFAULT_ORIGINATOR__HUB_ALWAYS  = 0x03u,
	DEFAULT_ORIGINATOR__GARD_ALWAYS = 0x04u,
};

enum originator_id {
	ORIGINATOR_ID__HUB  = 0xABu,
	ORIGINATOR_ID__GARD = 0xCDu,
};

#if (GARD_HUB_IFACE_VER == 1)
/**
 * Ensure these structures are not padded as they are exchanged by code running
 * on different architectures.
 * RISC-V firmware does not support unaligned access, so GARD firmware needs to
 * memcpy individual fields in a locally padded structure for use.
 */

#pragma pack(1)

struct _host_requests {
	/* Command identifier having a value from enum host_request_command_ids */
	uint8_t command_id;

	union {
		/* Point where the command body starts. */
		uint8_t command_body[1];

		/**
		 * struct send_data_to_gard_for_offset_request is to be used when
		 * command_id is SEND_DATA_TO_GARD_FOR_OFFSET.
		 */
		struct _send_data_to_gard_for_offset_request {
			struct {
				/* Offset address in GARD memory map. */
				uint32_t offset_address;

				/* Size of data to be sent to GARD */
				uint32_t data_size;

				/* Control code from enum control_codes */
				uint16_t control_code;

				/* Maximum Transmission Unit size */
				uint16_t mtu_size;
			} cmd;

			/* Data to be sent to GARD */
			uint8_t data[0];

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;

				/* CRC for data integrity */
				uint32_t opt_crc;
			} eod;
		} send_data_to_gard_for_offset_request;

		/**
		 * struct recv_data_from_gard_at_offset_request is to be used when
		 * command_id is RECV_DATA_FROM_GARD_AT_OFFSET.
		 */
		struct _recv_data_from_gard_at_offset_request {
			/* Offset address in GARD memory map. */
			uint32_t offset_address;

			/* Size of data to be received from GARD; */
			uint32_t data_size;

			/* Control code as defined in enum control_codes */
			uint16_t control_code;

			/* Maximum Transmission Unit size */
			uint16_t mtu_size;
		} recv_data_from_gard_at_offset_request;

		/**
		 * struct read_reg_value_from_gard_at_offset_request is to be used when
		 * command_id is READ_REG_VALUE_FROM_GARD_AT_OFFSET.
		 */
		struct _read_reg_value_from_gard_at_offset_request {
			/* Offset address in GARD memory map. */
			uint32_t offset_address;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} read_reg_value_from_gard_at_offset_request;

		/**
		 * struct write_reg_value_to_gard_at_offset_request is to be used when
		 * command_id is WRITE_REG_VALUE_TO_GARD_AT_OFFSET.
		 */
		struct _write_reg_value_to_gard_at_offset_request {
			/* Offset address in GARD memory map. */
			uint32_t offset_address;

			/* 32-bit data to be written to GARD */
			uint32_t data;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} write_reg_value_to_gard_at_offset_request;

		/**
		 * struct get_ml_engine_status_request is to be used when command_id is
		 * GET_ML_ENGINE_STATUS.
		 */
		struct _get_ml_engine_status_request {
			/* ID of the ML engine to get status for */
			uint32_t engine_id;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} get_ml_engine_status_request;

		struct _gard_discovery_request {
			/* No data present in the discovery command body */
			uint32_t dummy[0];
		} gard_discovery_request;

		/**
		 * struct capture_rescaled_image_request is to be used when
		 * command_id is CAPTURE_RESCALED_IMAGE.
		 */
		struct _capture_rescaled_image_request {
			/* Camera to take the image. */
			uint8_t camera_id;

			/* Pad bytes. */
			uint16_t rsvd1;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} capture_rescaled_image_request;

		/**
		 * struct pause_pipeline_request is to be used when
		 * command_id is PAUSE_PIPELINE.
		 */
		struct _pause_pipeline_request {
			/* Camera whose pipeline to pause. */
			uint8_t camera_id;

			/* Pad bytes. */
			uint16_t rsvd1;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} pause_pipeline_request;

		/**
		 * struct resume_pipeline_request is to be used when
		 * command_id is RESUME_PIPELINE.
		 */
		struct _resume_pipeline_request {
			/* Camera whose pipeline to resume. */
			uint8_t camera_id;

			/* Pad bytes. */
			uint16_t rsvd1;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} resume_pipeline_request;

		/**
		 * struct get_firmware_version_request is to be used when
		 * command_id is GET_FIRMWARE_VERSION.
		 */
		struct _get_firmware_version_request {
			/* No data present in the get firmware version command body */
			uint32_t dummy[0];
		} get_firmware_version_request;

		/**
		 * struct get_supported_commands_list_request is to be used when
		 * command_id is GET_SUPPORTED_CMNDS_LIST.
		 */
		struct _get_supported_commands_list_request {
			/**
			 * No data present in the get supported commands list command body
			 */
			uint32_t dummy[0];
		} get_supported_commands_list_request;

		/**
		 * struct get_supported_sub_commands_list_request is to be used when
		 * command_id is GET_SUPPORTED_SUB_CMNDS_LIST.
		 */
		struct _get_supported_sub_commands_list_request {
			/**
			 * No data present in the get supported sub commands list command
			 * body
			 */
			uint32_t dummy[0];
		} get_supported_sub_commands_list_request;

		/**
		 * struct upgrade_firmware_request is to be used when
		 * command_id is UPGRADE_FIRMWARE.
		 */
		struct _upgrade_firmware_request {
			/* Sub-command identifier */
			uint8_t sub_command_id;

			union {
				struct {
					/* Bitstream in image. */
					uint16_t is_bitstream_included : 1;

					/* Pad bytes */
					uint16_t rsvd1                 : 15;

					/* Size of the firmware */
					uint32_t firmware_size;

					/* Pad bytes */
					uint32_t rsvd2[2];
				} get_xfer_information;

				struct {
					/* Pad bytes */
					uint16_t rsvd1;

					/* Buffer address containing data */
					uint32_t buffer_address;

					/* Size of this data packet */
					uint32_t bytes_to_write;

					/* Flash address to write to */
					uint32_t flash_address;
				} write_firmware_to_flash;

				struct {
					/* Pad bytes */
					uint8_t rsvd1[14];
				} is_operation_complete;
			};
		} upgrade_firmware_request;
	};
};

struct _host_responses {
	union {
		/**
		 * struct send_data_to_gard_for_offset_response is to be used for
		 * sending an optional ACK as response to command
		 * SEND_DATA_TO_GARD_FOR_OFFSET.
		 */
		struct _send_data_to_gard_for_offset_response {
			/* Optional ACK byte to send after command execution */
			uint8_t opt_ack;
		} send_data_to_gard_for_offset_response;

		/**
		 * struct recv_data_from_gard_at_offset_response is to be used when
		 * command_id is RECV_DATA_FROM_GARD_AT_OFFSET.
		 */
		struct _recv_data_from_gard_at_offset_response {
			/* START OF DATA marker */
			uint32_t start_of_data_marker;

			/* Size of data sent by GARD; */
			uint32_t data_size;

			/* Data received from GARD */
			uint8_t data[0];

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;

				/* CRC for data integrity */
				uint32_t opt_crc;
			} eod;
		} recv_data_from_gard_at_offset_response;

		/**
		 * struct read_reg_value_from_gard_at_offset_response is to be used when
		 * command_id is READ_REG_VALUE_FROM_GARD_AT_OFFSET.
		 */
		struct _read_reg_value_from_gard_at_offset_response {
			/* START OF DATA marker */
			uint32_t start_of_data_marker;

			/* Register value read from GARD at specified offset */
			uint32_t reg_value;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} read_reg_value_from_gard_at_offset_response;

		/**
		 * struct write_reg_value_to_gard_at_offset_response is to be used when
		 * command_id is WRITE_REG_VALUE_TO_GARD_AT_OFFSET.
		 */
		struct _write_reg_value_to_gard_at_offset_response {
			/* ACK byte to send after write completion */
			uint8_t ack;
		} write_reg_value_to_gard_at_offset_response;

		/**
		 * struct get_ml_engine_status_response is to be used when command_id is
		 * GET_ML_ENGINE_STATUS.
		 */
		struct _get_ml_engine_status_response {
			/* START OF DATA marker */
			uint32_t start_of_data_marker;

			/* Status code of the ML engine */
			uint32_t status_code;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} get_ml_engine_status_response;

		/**
		 * struct gard_discovery_response is to be used when command_id is
		 * DISCOVERY.
		 */
		struct _gard_discovery_response {
			/* START OF DATA marker */
			uint32_t start_of_data_marker;

			/* Signature for discovery response */
			uint8_t signature[10];

			/* END OF DATA marker */
			uint32_t end_of_data_marker;

			/* No data present in the discovery response body */
		} gard_discovery_response;

		/**
		 * struct capture_rescaled_image_response is to be used when
		 * command_id is CAPTURE_RESCALED_IMAGE.
		 */
		struct _capture_rescaled_image_response {
			/* START OF DATA marker */
			uint32_t start_of_data_marker;

			/* Image buffer address. */
			uint32_t image_buffer_address;

			/* Size of the captured image. */
			uint32_t image_buffer_size;

			/* Horizontal size of the image. */
			uint16_t h_size;

			/* Vertical size of the image. */
			uint16_t v_size;

			/* Definition from enum image_formats */
			uint32_t image_format;

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;
			} eod;
		} capture_rescaled_image_response;

		/**
		 * struct pause_pipeline_response is to be used when
		 * command_id is PAUSE_PIPELINE.
		 */
		struct _pause_pipeline_response {
			/* Pipeline pause status */
			uint8_t ack_or_nak;
		} pause_pipeline_response;

		/**
		 * struct resume_pipeline_response is to be used when
		 * command_id is RESUME_PIPELINE.
		 */
		struct _resume_pipeline_response {
			/* Pipeline resume status */
			uint8_t ack_or_nak;
		} resume_pipeline_response;

		/**
		 * struct get_firmware_version_response is to be used when
		 * command_id is GET_FIRMWARE_VERSION.
		 */
		struct _get_firmware_version_response {
			/* START OF DATA marker */
			uint32_t start_of_data_marker;

			/* App Module ID defined above in enum app_module_ids. */
			uint16_t app_mod_id;

			/* FW Major version number. */
			uint16_t major_version_number;

			/* FW Minor version number. */
			uint16_t minor_version_number;

			/* FW Bugfix version number. */
			uint16_t bugfix_version_number;

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;
			} eod;
		} get_firmware_version_response;

		/**
		 * struct get_supported_commands_list_response is to be used when
		 * command_id is GET_SUPPORTED_CMNDS_LIST.
		 */
		struct _get_supported_commands_list_response {
			/* START OF DATA marker */
			uint32_t start_of_data_marker;

			/**
			 * Number of bytes in the supported commands bitmap. GARD ensures
			 * that the bytes after the end of bitmap is a multiple of 4.
			 */
			uint32_t num_bitmap_bytes;

			/* Bitmap array representing supported commands. */
			uint8_t bitmap[0];

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;
			} eod;
		} get_supported_commands_list_response;

		/**
		 * struct get_supported_sub_commands_list_response is to be used when
		 * command_id is GET_SUPPORTED_SUB_CMNDS_LIST.
		 */
		struct _get_supported_sub_commands_list_response {
			/* START OF DATA marker */
			uint32_t start_of_data_marker;

			/**
			 * Number of bytes in the supported sub commands bitmap.
			 * GARD ensures that the bytes after the end of bitmap is a
			 * multiple of 4.
			 */
			uint32_t num_bitmap_bytes;

			/* Bitmap array representing supported sub commands. */
			uint8_t bitmap[0];

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;
			} eod;
		} get_supported_sub_commands_list_response;

		/**
		 * struct upgrade_firmware_response is to be used when
		 * command_id is UPGRADE_FIRMWARE.
		 */
		struct _upgrade_firmware_response {
			union {
				struct {
					/* START OF DATA marker */
					uint32_t start_of_data_marker;

					/* Buffer address. */
					uint32_t buffer_address;

					/* Buffer size. */
					uint32_t buffer_size;

					/* Flash address. */
					uint32_t flash_address;

					struct {
						/* END OF DATA marker */
						uint32_t end_of_data_marker;
					} eod;
				} get_xfer_information_response;

				struct {
					/* ACK */
					uint8_t ack;
				} write_firmware_to_flash_response;

				struct {
					/* 1 if complete, 0 otherwise */
					uint8_t operation_status;
				} is_operation_complete_response;
			};
		} upgrade_firmware_response;
	};
};

#pragma pack()

#else

/**
 * Ensure these structures are not padded as they are exchanged by code running
 * on different architectures.
 * RISC-V firmware does not support unaligned access, so GARD firmware needs to
 * memcpy individual fields in a locally padded structure for use.
 */

#pragma pack(1)

struct _gard_hub_requests {
	/* Command identifier having a value from enum host_request_command_ids */
	uint8_t command_id;

	union {
		/* Point where the command body starts. */
		uint8_t command_body[1];

		/**
		 * struct send_data_to_gard_for_offset_request is to be used when
		 * command_id is SEND_DATA_TO_GARD_FOR_OFFSET.
		 */
		struct _send_data_to_gard_for_offset_request {
			struct {
				/* Offset address in GARD memory map. */
				uint32_t offset_address;

				/* Size of data to be sent to GARD */
				uint32_t data_size;

				/* Control code from enum control_codes */
				uint16_t control_code;

				/* Maximum Transmission Unit size */
				uint16_t mtu_size;
			} cmd;

			/* Data to be sent to GARD */
			uint8_t data[0];

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;

				/* CRC for data integrity */
				uint32_t opt_crc;
			} eod;
		} send_data_to_gard_for_offset_request;

		/**
		 * struct recv_data_from_gard_at_offset_request is to be used when
		 * command_id is RECV_DATA_FROM_GARD_AT_OFFSET.
		 */
		struct _recv_data_from_gard_at_offset_request {
			/* Offset address in GARD memory map. */
			uint32_t offset_address;

			/* Size of data to be received from GARD; */
			uint32_t data_size;

			/* Control code as defined in enum control_codes */
			uint16_t control_code;

			/* Maximum Transmission Unit size */
			uint16_t mtu_size;
		} recv_data_from_gard_at_offset_request;

		/**
		 * struct read_reg_value_from_gard_at_offset_request is to be used when
		 * command_id is READ_REG_VALUE_FROM_GARD_AT_OFFSET.
		 */
		struct _read_reg_value_from_gard_at_offset_request {
			/* Offset address in GARD memory map. */
			uint32_t offset_address;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} read_reg_value_from_gard_at_offset_request;

		/**
		 * struct write_reg_value_to_gard_at_offset_request is to be used when
		 * command_id is WRITE_REG_VALUE_TO_GARD_AT_OFFSET.
		 */
		struct _write_reg_value_to_gard_at_offset_request {
			/* Offset address in GARD memory map. */
			uint32_t offset_address;

			/* 32-bit data to be written to GARD */
			uint32_t data;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} write_reg_value_to_gard_at_offset_request;

		/**
		 * struct get_ml_engine_status_request is to be used when command_id is
		 * GET_ML_ENGINE_STATUS.
		 */
		struct _get_ml_engine_status_request {
			/* ID of the ML engine to get status for */
			uint32_t engine_id;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} get_ml_engine_status_request;

		struct _gard_discovery_request {
			/* No data present in the discovery command body */
			uint32_t dummy[0];
		} gard_discovery_request;

		/**
		 * struct capture_rescaled_image_request is to be used when
		 * command_id is CAPTURE_RESCALED_IMAGE.
		 */
		struct _capture_rescaled_image_request {
			/* Camera to take the image. */
			uint8_t camera_id;

			/* Pad bytes. */
			uint16_t rsvd1;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} capture_rescaled_image_request;

		/**
		 * struct pause_pipeline_request is to be used when
		 * command_id is PAUSE_PIPELINE.
		 */
		struct _pause_pipeline_request {
			/* Camera whose pipeline to pause. */
			uint8_t camera_id;

			/* Pad bytes. */
			uint16_t rsvd1;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} pause_pipeline_request;

		/**
		 * struct resume_pipeline_request is to be used when
		 * command_id is RESUME_PIPELINE.
		 */
		struct _resume_pipeline_request {
			/* Camera whose pipeline to resume. */
			uint8_t camera_id;

			/* Pad bytes. */
			uint16_t rsvd1;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} resume_pipeline_request;

		/**
		 * struct get_firmware_version_request is to be used when
		 * command_id is GET_FIRMWARE_VERSION.
		 */
		struct _get_firmware_version_request {
			/* No data present in the get firmware version command body */
			uint32_t dummy[0];
		} get_firmware_version_request;

		/**
		 * struct get_supported_commands_list_request is to be used when
		 * command_id is GET_SUPPORTED_CMNDS_LIST.
		 */
		struct _get_supported_commands_list_request {
			/**
			 * No data present in the get supported commands list command body
			 */
			uint32_t dummy[0];
		} get_supported_commands_list_request;

		/**
		 * struct get_supported_sub_commands_list_request is to be used when
		 * command_id is GET_SUPPORTED_SUB_CMNDS_LIST.
		 */
		struct _get_supported_sub_commands_list_request {
			/**
			 * No data present in the get supported sub commands list command
			 * body
			 */
			uint32_t dummy[0];
		} get_supported_sub_commands_list_request;

		/**
		 * struct upgrade_firmware_request is to be used when
		 * command_id is UPGRADE_FIRMWARE.
		 */
		struct _upgrade_firmware_request {
			/* Sub-command identifier */
			uint8_t sub_command_id;

			union {
				struct {
					/* Bitstream in image. */
					uint16_t is_bitstream_included : 1;

					/* Pad bytes */
					uint16_t rsvd1                 : 15;

					/* Size of the firmware */
					uint32_t firmware_size;

					/* Pad bytes */
					uint32_t rsvd2[2];
				} get_xfer_information;

				struct {
					/* Pad bytes */
					uint16_t rsvd1;

					/* Buffer address containing data */
					uint32_t buffer_address;

					/* Size of this data packet */
					uint32_t bytes_to_write;

					/* Flash address to write to */
					uint32_t flash_address;
				} write_firmware_to_flash;

				struct {
					/* Pad bytes */
					uint8_t rsvd1[14];
				} is_operation_complete;
			};
		} upgrade_firmware_request;

		/**
		 * struct originator_sync_request is to be used when command_id is
		 * ORIGINATOR_SYNC. This is used by GARD/HUB before initiating
		 * an App Layer send command.
		 */
		struct _originator_sync_request {
			/* ID of the originator (sender of this request) */
			uint8_t originator_id;
		} originator_sync_request;

		/**
		 * struct gard_app_layer_send is to be used when command_id is
		 * GARD_APP_LAYER_SEND. No response is expected from the peer App Layer.
		 */
		struct _gard_app_layer_send {
			/**
			 * Size of Payload in bytes. The payload is composed by the App
			 * layer that it wants to send to the peer App layer.
			 * Be careful when handling this field on architectures that do not
			 * support unaligned access.
			 */
			uint8_t gard_app_layer_payload_size[3];

			/* The payload is the App layer data. */
			uint8_t gard_app_layer_payload[0];
		} gard_app_layer_send;

		/**
		 * struct hub_app_layer_send is to be used when command_id is
		 * HUB_APP_LAYER_SEND. No response is expected from the peer App Layer.
		 */
		struct _hub_app_layer_send {
			/**
			 * Size of Payload in bytes. The payload is composed by the App
			 * layer that it wants to send to the peer App layer.
			 * Be careful when handling this field on architectures that do not
			 * support unaligned access.
			 */
			uint8_t hub_app_layer_payload_size[3];

			/* The payload is the App layer data. */
			uint8_t hub_app_layer_payload[0];
		} hub_app_layer_send;

		/**
		 * struct gard_app_layer_request is to be used when command_id is
		 * GARD_APP_LAYER_REQUEST. A response is expected from the peer App
		 * Layer.
		 */
		struct _gard_app_layer_request {
			/**
			 * Size of Payload in bytes. The payload is composed by the App
			 * layer that it wants to send to the peer App layer.
			 * Be careful when handling this field on architectures that do not
			 * support unaligned access.
			 */
			uint8_t gard_app_layer_payload_size[3];

			/* The payload is the App layer data. */
			uint8_t gard_app_layer_payload[0];
		} gard_app_layer_request;

		/**
		 * struct hub_app_layer_request is to be used when command_id is
		 * HUB_APP_LAYER_REQUEST. A response is expected from the peer App
		 * Layer.
		 */
		struct _hub_app_layer_request {
			/**
			 * Size of Payload in bytes. The payload is composed by the App
			 * layer that it wants to send to the peer App layer.
			 * Be careful when handling this field on architectures that do not
			 * support unaligned access.
			 */
			uint8_t hub_app_layer_payload_size[3];

			/* The payload is the App layer data. */
			uint8_t hub_app_layer_payload[0];
		} hub_app_layer_request;

		/**
		 * struct coordinate_default_originator_request is to be used when
		 * command_id is COORDINATE_DEFAULT_ORIGINATOR.
		 */
		struct _coordinate_default_originator_request {
			/* Currently active Default originator. */
			uint8_t default_originator;

			/* Pad bytes. */
			uint8_t rsvd1[2];

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} coordinate_default_originator_request;
	};
};

struct _gard_hub_responses {
	/* Response identifier should be the same as the request identifier */
	uint8_t response_id;

	union {
		/**
		 * union send_data_to_gard_for_offset_response is to be used for
		 * sending an optional ACK as response to command
		 * SEND_DATA_TO_GARD_FOR_OFFSET.
		 */
		union _send_data_to_gard_for_offset_response {
			struct {
				/* Flag to request HUB to start sending data to GARD. */
				uint8_t send_data_start;
			} start_signal;

			struct {
				/* Optional ACK byte to send after command execution */
				uint8_t opt_ack;
			} ack;
		} send_data_to_gard_for_offset_response;

		/**
		 * struct recv_data_from_gard_at_offset_response is to be used when
		 * command_id is RECV_DATA_FROM_GARD_AT_OFFSET.
		 */
		struct _recv_data_from_gard_at_offset_response {
			/* Padding bytes */
			uint8_t rsvd1[3];

			/* Size of data sent by GARD; */
			uint32_t data_size;

			/* Data received from GARD */
			uint8_t data[0];

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;

				/* CRC for data integrity */
				uint32_t opt_crc;
			} eod;
		} recv_data_from_gard_at_offset_response;

		/**
		 * struct read_reg_value_from_gard_at_offset_response is to be used when
		 * command_id is READ_REG_VALUE_FROM_GARD_AT_OFFSET.
		 */
		struct _read_reg_value_from_gard_at_offset_response {
			/* Padding bytes */
			uint8_t rsvd1[3];

			/* Register value read from GARD at specified offset */
			uint32_t reg_value;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} read_reg_value_from_gard_at_offset_response;

		/**
		 * struct write_reg_value_to_gard_at_offset_response is to be used when
		 * command_id is WRITE_REG_VALUE_TO_GARD_AT_OFFSET.
		 */
		struct _write_reg_value_to_gard_at_offset_response {
			/* ACK byte to send after write completion */
			uint8_t ack;
		} write_reg_value_to_gard_at_offset_response;

		/**
		 * struct get_ml_engine_status_response is to be used when command_id is
		 * GET_ML_ENGINE_STATUS.
		 */
		struct _get_ml_engine_status_response {
			/* Padding bytes */
			uint8_t rsvd1[3];

			/* Status code of the ML engine */
			uint32_t status_code;

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} get_ml_engine_status_response;

		/**
		 * struct gard_discovery_response is to be used when command_id is
		 * DISCOVERY.
		 */
		struct _gard_discovery_response {
			/* Padding bytes */
			uint8_t rsvd1[1];

			/* Signature for discovery response */
			uint8_t signature[10];

			/* END OF DATA marker */
			uint32_t end_of_data_marker;
		} gard_discovery_response;

		/**
		 * struct capture_rescaled_image_response is to be used when
		 * command_id is CAPTURE_RESCALED_IMAGE.
		 */
		struct _capture_rescaled_image_response {
			/* Padding bytes */
			uint8_t rsvd1[3];

			/* Image buffer address. */
			uint32_t image_buffer_address;

			/* Size of the captured image. */
			uint32_t image_buffer_size;

			/* Horizontal size of the image. */
			uint16_t h_size;

			/* Vertical size of the image. */
			uint16_t v_size;

			/* Definition from enum image_formats */
			uint32_t image_format;

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;
			} eod;
		} capture_rescaled_image_response;

		/**
		 * struct pause_pipeline_response is to be used when
		 * command_id is PAUSE_PIPELINE.
		 */
		struct _pause_pipeline_response {
			/* Pipeline pause status */
			uint8_t ack_or_nak;
		} pause_pipeline_response;

		/**
		 * struct resume_pipeline_response is to be used when
		 * command_id is RESUME_PIPELINE.
		 */
		struct _resume_pipeline_response {
			/* Pipeline resume status */
			uint8_t ack_or_nak;
		} resume_pipeline_response;

		/**
		 * struct get_firmware_version_response is to be used when
		 * command_id is GET_FIRMWARE_VERSION.
		 */
		struct _get_firmware_version_response {
			/* Padding bytes */
			uint8_t rsvd1[3];

			/* App Module ID defined above in enum app_module_ids. */
			uint16_t app_mod_id;

			/* FW Major version number. */
			uint16_t major_version_number;

			/* FW Minor version number. */
			uint16_t minor_version_number;

			/* FW Bugfix version number. */
			uint16_t bugfix_version_number;

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;
			} eod;
		} get_firmware_version_response;

		/**
		 * struct get_supported_commands_list_response is to be used when
		 * command_id is GET_SUPPORTED_CMNDS_LIST.
		 */
		struct _get_supported_commands_list_response {
			/* Padding bytes */
			uint8_t rsvd1[3];

			/**
			 * Number of bytes in the supported commands bitmap. GARD ensures
			 * that the bytes after the end of bitmap is a multiple of 4.
			 */
			uint32_t num_bitmap_bytes;

			/* Bitmap array representing supported commands. */
			uint8_t bitmap[0];

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;
			} eod;
		} get_supported_commands_list_response;

		/**
		 * struct get_supported_sub_commands_list_response is to be used when
		 * command_id is GET_SUPPORTED_SUB_CMNDS_LIST.
		 */
		struct _get_supported_sub_commands_list_response {
			/* Padding bytes */
			uint8_t rsvd1[3];

			/**
			 * Number of bytes in the supported sub commands bitmap.
			 * GARD ensures that the bytes after the end of bitmap is a
			 * multiple of 4.
			 */
			uint32_t num_bitmap_bytes;

			/* Bitmap array representing supported sub commands. */
			uint8_t bitmap[0];

			struct {
				/* END OF DATA marker */
				uint32_t end_of_data_marker;
			} eod;
		} get_supported_sub_commands_list_response;

		/**
		 * struct upgrade_firmware_response is to be used when
		 * command_id is UPGRADE_FIRMWARE.
		 */
		struct _upgrade_firmware_response {
			union {
				struct {
					/* Padding bytes */
					uint8_t rsvd1[3];

					/* Buffer address. */
					uint32_t buffer_address;

					/* Buffer size. */
					uint32_t buffer_size;

					/* Flash address. */
					uint32_t flash_address;

					struct {
						/* END OF DATA marker */
						uint32_t end_of_data_marker;
					} eod;
				} get_xfer_information_response;

				struct {
					/* ACK */
					uint8_t ack;
				} write_firmware_to_flash_response;

				struct {
					/* 1 if complete, 0 otherwise */
					uint8_t operation_status;
				} is_operation_complete_response;
			};
		} upgrade_firmware_response;

		/**
		 * struct originator_sync_response is to be used when command_id is
		 * ORIGINATOR_SYNC. This is used by GARD/HUB as a response to the
		 * originator sync request.
		 */
		struct _originator_sync_response {
			/* ID of the originator (receiver of this response) */
			uint8_t originator_id;
		} originator_sync_response;

		/**
		 * struct gard_app_layer_response is to be used when command_id is
		 * HUB_APP_LAYER_REQUEST. This is because GARD App Layer provides
		 * response for the HUB App Layer's request.
		 */
		struct _gard_app_layer_response {
			/**
			 * Size of Payload in bytes. The payload is composed by the App
			 * layer in response to the request that was sent by the peer App
			 * layer. Be careful when handling this field on architectures that
			 * do not support unaligned access.
			 */
			uint8_t gard_app_layer_payload_size[3];

			/**
			 * The payload that contains the response data. The contents here
			 * are one of the structures within _gard_app_layer_response and are
			 * selected depending on the original request that was sent by the
			 * peer App layer.
			 */
			uint8_t gard_app_layer_payload[0];
		} gard_app_layer_response;

		/**
		 * struct hub_app_layer_response is to be used when command_id is
		 * GARD_APP_LAYER_REQUEST. This is because HUB App Layer provides
		 * response for the GARD App Layer's request.
		 */
		struct _hub_app_layer_response {
			/**
			 * Size of Payload in bytes. The payload is composed by the App
			 * layer in response to the request that was sent by the peer App
			 * layer. Be careful when handling this field on architectures that
			 * do not support unaligned access.
			 */
			uint8_t hub_app_layer_payload_size[3];

			/**
			 * The payload that contains the response data. The contents here
			 * are one of the structures within _hub_app_layer_response and are
			 * selected depending on the original request that was sent by the
			 * peer App layer.
			 */
			uint8_t hub_app_layer_payload[0];
		} hub_app_layer_response;

		/**
		 * struct coordinate_default_originator_response is to be used when
		 * command_id is COORDINATE_DEFAULT_ORIGINATOR.
		 */
		struct _coordinate_default_originator_response {
			/* ACK */
			uint8_t ack;
		} coordinate_default_originator_response;
	};
};

#pragma pack()

/*******************************************************************************
 * The following section is the interaction between the App and the App Module.
 * In the following lines App Layers refer to either App and App Module.
 ******************************************************************************/

/**
 * The following requests are used by App layers running over HUB and GARD. Only
 * the App and App Module layers know how to interpret their respective
 * contents.
 */
enum hub_app_layer_request_ids {
	APP_MODULE_GET_TYPE         = 0x00u,
	APP_MODULE_GET_FEATURE_LIST = 0x01u,
	GENERATE_FACE_ID            = 0x02u,
	DELETE_FACE_ID              = 0x03u,
	START_FACE_IDENTIFICATION   = 0x04u,
	STOP_FACE_IDENTIFICATION    = 0x05u,
	GET_FACE_ID_LIST            = 0x06u,
	HMI_METADATA                = 0x07u,
	START_QR_MONITORING         = 0x08u,
	STOP_QR_MONITORING          = 0x09u,
	QR_CODE_IMAGE               = 0x0Au,

	/**
	 * The following is used as Response when the corresponding App Layer is not
	 * available.
	 */
	UNSUPPPORTED                = 0xFFu,
};

#pragma pack(1)

/**
 * The structures contained within _hub_app_layer_requests are used to send
 * requests from the App to the App Module. These structures are added as a part
 * of struct _gard_hub_requests.hub_app_layer_request and are placed at the
 * location _gard_hub_requests.hub_app_layer_request.hub_app_layer_payload.
 */
struct _hub_app_layer_requests {
	/**
	 * Request identifier having a value from enum hub_app_layer_request_ids.
	 * This is the only common field between all the structures and is used to
	 * identify the request that is sent by the App layer.
	 */
	uint8_t hub_app_layer_request_id;

	union {
		/**
		 * struct app_module_get_type_request is to be used when
		 * hub_app_layer_request_id is APP_MODULE_GET_TYPE.
		 */
		struct _app_module_get_type_request {
			/* No data present in the app module get type request body */
			uint32_t dummy[0];
		} app_module_get_type_request;

		/**
		 * struct app_module_get_feature_list_request is to be used when
		 * hub_app_layer_request_id is APP_MODULE_GET_FEATURE_LIST.
		 */
		struct _app_module_get_feature_list_request {
			/**
			 * No data present in the app module get feature list request body
			 */
			uint32_t dummy[0];
		} app_module_get_feature_list_request;

		/**
		 * struct generate_face_id_request is to be used when
		 * hub_app_layer_request_id is GENERATE_FACE_ID.
		 */
		struct _generate_face_id_request {
			/* No data present in the generate face id request body */
			uint32_t dummy[0];
		} generate_face_id_request;

		/**
		 * struct delete_face_id_request is to be used when
		 * hub_app_layer_request_id is DELETE_FACE_ID.
		 */
		struct _delete_face_id_request {
			/* Pad bytes. */
			uint8_t rsvd1;

			/* Face ID to delete. */
			uint16_t face_id;
		} delete_face_id_request;

		/**
		 * struct start_face_identification_request is to be used when
		 * hub_app_layer_request_id is START_FACE_IDENTIFICATION.
		 */
		struct _start_face_identification_request {
			/* No data present in the start face identification request body */
			uint32_t dummy[0];
		} start_face_identification_request;

		/**
		 * struct stop_face_identification_request is to be used when
		 * hub_app_layer_request_id is STOP_FACE_IDENTIFICATION.
		 */
		struct _stop_face_identification_request {
			/* No data present in the stop face identification request body */
			uint32_t dummy[0];
		} stop_face_identification_request;

		/**
		 * struct get_face_id_list_request is to be used when
		 * hub_app_layer_request_id is GET_FACE_ID_LIST.
		 */
		struct _get_face_id_list_request {
			/* No data present in the get face id list request body */
			uint32_t dummy[0];
		} get_face_id_list_request;

		/**
		 * struct start_qr_monitoring_request is to be used when
		 * hub_app_layer_request_id is START_QR_MONITORING.
		 */
		struct _start_qr_monitoring_request {
			/* No data present in the start qr monitoring request body */
			uint32_t dummy[0];
		} start_qr_monitoring_request;

		/**
		 * struct stop_qr_monitoring_request is to be used when
		 * hub_app_layer_request_id is STOP_QR_MONITORING.
		 */
		struct _stop_qr_monitoring_request {
			/* No data present in the stop qr monitoring request body */
			uint32_t dummy[0];
		} stop_qr_monitoring_request;
	};
};

/**
 * The structures contained within _gard_app_layer_responses are used to send
 * responses from the App Module to the App. These structures are sent as a
 * response to the host_app_layer_requests sent by the App.
 * These structures are added as a part of
 * struct _gard_hub_responses.gard_app_layer_response and are placed at the
 * location _gard_hub_responses.gard_app_layer_response.gard_app_layer_payload.
 */
struct _gard_app_layer_responses {
	/**
	 * Response identifier having a value from enum gard_app_layer_send_commands
	 * since the response contains the same ID as in the send packet.
	 */
	uint8_t gard_app_layer_response_id;

	union {
		/**
		 * struct app_mod_get_type_response is to be used when
		 * hub_app_layer_request_id is APP_MOD_GET_TYPE.
		 */
		struct _app_mod_get_type_response {
			/* App Module type. */
			uint8_t app_mod_type;
		} app_mod_get_type_response;

		/**
		 * struct app_mod_get_feature_list_response is to be used when
		 * hub_app_layer_request_id is APP_MOD_GET_FEATURE_LIST.
		 */
		struct _app_mod_get_feature_list_response {
			/* Number of features supported by the app module. */
			uint8_t app_mod_feature_list_count;

			/* List of features supported by the app module. */
			uint8_t app_mod_feature_list[0];
		} app_mod_get_feature_list_response;

		/**
		 * struct generate_face_id_response is to be used when
		 * hub_app_layer_request_id is GENERATE_FACE_ID.
		 */
		struct _generate_face_id_response {
			/* Fraction bits value used for fixed point representation .*/
			uint8_t frac_bits;

			/* Generated Face ID. */
			uint16_t face_id;

			/* Top coordinate of the face identified by above face ID`. */
			uint32_t face_top;

			/* Left coordinate of the face identified by above face ID. */
			uint32_t face_left;

			/* Bottom coordinate of the face identified by above face ID. */
			uint32_t face_bottom;

			/* Right coordinate of the face identified by above face ID. */
			uint32_t face_right;
		} generate_face_id_response;

		/**
		 * struct delete_face_id_response is to be used when
		 * hub_app_layer_request_id is DELETE_FACE_ID.
		 */
		struct _delete_face_id_response {
			/* ACK or NAK indicating status of the delete operation. */
			uint8_t ack_or_nak;
		} delete_face_id_response;

		/**
		 * struct start_face_identification_response is to be used when
		 * hub_app_layer_request_id is START_FACE_IDENTIFICATION.
		 */
		struct _start_face_identification_response {
			/**
			 * ACK or NAK indicating status of the start face identification
			 * operation.
			 */
			uint8_t ack_or_nak;
		} start_face_identification_response;

		/**
		 * struct stop_face_identification_response is to be used when
		 * hub_app_layer_request_id is STOP_FACE_IDENTIFICATION.
		 */
		struct _stop_face_identification_response {
			/**
			 * ACK or NAK indicating status of the stop face identification
			 * operation.
			 */
			uint8_t ack_or_nak;
		} stop_face_identification_response;

		/**
		 * struct get_face_id_list_response is to be used when
		 * hub_app_layer_request_id is GET_FACE_ID_LIST.
		 */
		struct _get_face_id_list_response {
			/* Number of face IDs (not bytes) in the following list. */
			uint8_t face_id_list_count;

			/* List of face IDs. */
			uint16_t face_id_list[0];
		} get_face_id_list_response;

		/**
		 * struct start_qr_monitoring_response is to be used when
		 * hub_app_layer_request_id is START_QR_MONITORING.
		 */
		struct _start_qr_monitoring_response {
			/**
			 * ACK or NAK indicating status of the start qr monitoring
			 * operation.
			 */
			uint8_t ack_or_nak;
		} start_qr_monitoring_response;

		/**
		 * struct stop_qr_monitoring_response is to be used when
		 * hub_app_layer_request_id is STOP_QR_MONITORING.
		 */
		struct _stop_qr_monitoring_response {
			/**
			 * ACK or NAK indicating status of the stop qr monitoring operation.
			 */
			uint8_t ack_or_nak;
		} stop_qr_monitoring_response;

		/**
		 * struct unsupported_response is to be used when
		 * hub_app_layer_request_id is UNSUPPORTED. App Module returns
		 * UNSUPPORTED response when it encounters an App Layer command that it
		 * does not support. Additionally, GARD sends this response when App
		 * Module is not available or non-responsive.
		 */
		struct _unsupported_response {
			/**
			 * No data present in the unsupported response body
			 */
			uint32_t dummy[0];
		} unsupported_response;
	};
};

/**
 * The structures contained within _gard_app_layer_sends are used to send
 * command/data from the App Module to the App. These structures are sent as a
 * part of struct _gard_hub_requests.gard_app_layer_send and are placed at the
 * location _gard_hub_requests.gard_app_layer_send.gard_app_layer_payload.
 */
struct _gard_app_layer_sends {
	/**
	 * Request ID of the command/data to be sent to the App.
	 */
	uint8_t gard_app_layer_send_command;

	union {
		/**
		 * struct hmi_metadata_send is to be used when
		 * gard_app_layer_send_command is HMI_METADATA.
		 */
		struct _hmi_metadata_send {
			/**
			 * Size of the metadata. This field is 3 bytes long and should be
			 * copied to a 32-bit variable before interpreting it.
			 */
			uint8_t metadata_size[3];

			/* HMI metadata to be sent to the App. */
			uint8_t metadata[0];
		} hmi_metadata_send;

		/**
		 * struct qr_code_image_send is to be used when
		 * gard_app_layer_send_command is QR_CODE_IMAGE.
		 */
		struct _qr_code_image_send {
			/* Type of the QR image -> Grayscale or RGB. */
			uint8_t qr_image_type;

			/* Width of the QR image. */
			uint16_t qr_image_width;

			/* Height of the QR image. */
			uint16_t qr_image_height;

			/* The QR image. */
			uint8_t qr_image[0];
		} qr_code_image_send;
	};
};

#pragma pack()

#endif /* (GARD_HUB_IFACE_VER == 1) */

#endif /* GARD_HUB_IFACE_H */
