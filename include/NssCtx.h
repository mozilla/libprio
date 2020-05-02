/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs, Shivam Balikondwar
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __NSSCTX_H_
#define __NSSCTX_H_

#include <nss.h>

typedef struct
{
  // Function Pointer to initiate Nss struct contexts.
  NSSInitContext* (*Nss_InitContext)(
    const char* configdir, const char* certPrefix, const char* keyPrefix,
    const char* secmodName, NSSInitParameters* initParams, PRUint32 flags);

  // Function Pointer to check initialization of Nss struct.
  PRBool (*Nss_Isinit)();

  // Function Pointer to shutdown the Nss random number generators.
  SECStatus (*Nss_Shutdown)(NSSInitContext* context);
} nss_struct;

// Function for assigning the nss callbacks
void nss_callbk_init(nss_struct* nss_pointer);

#endif /* __NSSCTX_H_ */