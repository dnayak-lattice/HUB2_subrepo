/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __KT_APP_FACE_DB_H__
#define __KT_APP_FACE_DB_H__

#include "kt_app.h"
#include "kt_app_logger.h"
#include "os_funcs.h"

/**
 * Initialize the face database
 *
 * @param app_handle The App handle
 * @param face_db_file_path The path to the face database file
 *
 * @return void
 */
void kt_app_face_db_init(App_Handle app_handle, const char *face_db_file_path);

/**
 * Finalize the face database
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_face_db_fini(App_Handle app_handle);

/**
 * Update a face profile in the face database
 *
 * @param app_handle The App handle
 * @param face_id The ID of the face to update
 * @param face_name The name of the face
 *
 * @return bool True if the face profile was updated, false otherwise
 */
bool kt_app_face_db_update_face_profile(App_Handle  app_handle,
										uint32_t    face_id,
										const char *face_name);

/**
 * Add a face profile to the face database
 *
 * @param app_handle The App handle
 * @param face_id The ID of the face to add
 * @param face_name The name of the face
 *
 * @return bool True if the face profile was added, false otherwise
 */
bool kt_app_face_db_add_face_profile(App_Handle  app_handle,
									 uint32_t    face_id,
									 const char *face_name);

/**
 * Flush the face database to disk
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_face_db_flush_to_disk(App_Handle app_handle);

#endif /* __KT_APP_FACE_DB_H__ */