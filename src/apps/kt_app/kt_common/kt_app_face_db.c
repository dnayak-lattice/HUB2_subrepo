/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "kt_app_face_db.h"

/* Kt App Face Database Functions */

/**
 * Initialize the face database
 *
 * @param app_handle The App handle
 * @param face_db_file_path The path to the face database file
 *
 * @return void
 */
void kt_app_face_db_init(App_Handle app_handle, const char *face_db_file_path)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;

	os_snprintf(p_app_ctx->face_db_file_path,
				sizeof(p_app_ctx->face_db_file_path), "%s", face_db_file_path);

	/* Read/write without truncating; create empty file if it does not exist */
	p_app_ctx->face_db_file = fopen(p_app_ctx->face_db_file_path, "r+");
	if (p_app_ctx->face_db_file == NULL && errno == ENOENT) {
		p_app_ctx->face_db_file = fopen(p_app_ctx->face_db_file_path, "w+");
	}
	if (p_app_ctx->face_db_file == NULL) {
		os_pr_err("Failed to open face database file %s",
				  p_app_ctx->face_db_file_path);
		return;
	}

	cmd_log_write(p_app_ctx, "Reading face database file %s",
				  p_app_ctx->face_db_file_path);

	p_app_ctx->face_profile_count = 0;
	while (p_app_ctx->face_profile_count < KT_APP_MAX_FACE_PROFILES_ON_DISK) {
		struct kt_app_face_profile *fp =
			&p_app_ctx->face_profiles[p_app_ctx->face_profile_count];

		if (fscanf(p_app_ctx->face_db_file, "%u:%255[^\n]", &fp->face_id,
				   fp->face_name) != 2) {
			break;
		}
		fp->face_name[strcspn(fp->face_name, "\r\n")] = '\0';
		cmd_log_write(p_app_ctx, "--- Face profile[%d]: ID[%d], Name[%s]",
					  p_app_ctx->face_profile_count, fp->face_id,
					  fp->face_name);
		p_app_ctx->face_profile_count++;
	}
}

/**
 * Finalize the face database
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_face_db_fini(App_Handle app_handle)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;

	if (p_app_ctx->face_db_file != NULL) {
		fclose(p_app_ctx->face_db_file);
		p_app_ctx->face_db_file = NULL;
	}
}

/**
 * Update an already existing face profile in the face database
 *
 * @param app_handle The App handle
 * @param face_id The ID of the face to update
 * @param face_name The name of the face
 *
 * @return bool True if the face profile was updated, false otherwise
 */
bool kt_app_face_db_update_face_profile(App_Handle  app_handle,
										uint32_t    face_id,
										const char *face_name)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;

	/* If the face profile does not exist, return false */
	for (uint32_t i = 0; i < p_app_ctx->face_profile_count; i++) {
		if (p_app_ctx->face_profiles[i].face_id == face_id) {
			os_memcpy(p_app_ctx->face_profiles[i].face_name, (void *)face_name,
					  sizeof(p_app_ctx->face_profiles[i].face_name));
			return true;
		}
	}
	return false;
}

/**
 * This function adds a face profile to the in-memory database.
 * If the face profile already exists, it is overwritten.
 *
 * @param app_handle The App handle
 * @param face_id The ID of the face to add
 * @param face_name The name of the face
 *
 * @return bool True if the face profile was added, false otherwise
 */
bool kt_app_face_db_add_face_profile(App_Handle  app_handle,
									 uint32_t    face_id,
									 const char *face_name)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;

	if (p_app_ctx->face_profile_count >= KT_APP_MAX_FACE_PROFILES_ON_DISK) {
		os_pr_err("Face profile count exceeds limit of %d\n",
				  KT_APP_MAX_FACE_PROFILES_ON_DISK);
		return false;
	}

	/* If the face profile already exists, update it */
	for (uint32_t i = 0; i < p_app_ctx->face_profile_count; i++) {
		if (p_app_ctx->face_profiles[i].face_id == face_id) {
			os_memcpy(p_app_ctx->face_profiles[i].face_name, (void *)face_name,
					  sizeof(p_app_ctx->face_profiles[i].face_name));
			return true;
		}
	}

	/* If the face profile does not exist, add it */
	p_app_ctx->face_profiles[p_app_ctx->face_profile_count].face_id = face_id;
	os_memcpy(p_app_ctx->face_profiles[p_app_ctx->face_profile_count].face_name, (void *)face_name,
			  sizeof(p_app_ctx->face_profiles[p_app_ctx->face_profile_count].face_name));
	p_app_ctx->face_profile_count++;


	cmd_log_write(p_app_ctx, "Added face profile: ID[%d], Name[%s]", face_id,
				  face_name);
	return true;
}

/**
 * Flush the face database to disk
 *
 * @param app_handle The App handle
 *
 * @return void
 */
void kt_app_face_db_flush_to_disk(App_Handle app_handle)
{
	struct kt_app_ctx *p_app_ctx = (struct kt_app_ctx *)app_handle;

	if (p_app_ctx->face_db_file == NULL) {
		return;
	}

	rewind(p_app_ctx->face_db_file);
	for (uint32_t i = 0; i < p_app_ctx->face_profile_count; i++) {
		fprintf(p_app_ctx->face_db_file, "%u:%s\n",
				p_app_ctx->face_profiles[i].face_id,
				p_app_ctx->face_profiles[i].face_name);
	}
	fflush(p_app_ctx->face_db_file);
	{
		long flen = ftell(p_app_ctx->face_db_file);

		if (flen >= 0) {
			(void)ftruncate(fileno(p_app_ctx->face_db_file), (off_t)flen);
		}
	}

	os_pr_info("Flushed face database to disk\n");
	cmd_log_write(p_app_ctx, "Flushed face database to disk");
}
