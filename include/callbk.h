/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CALLBK_H_
#define __CALLBK_H_

#include <nss.h>

typedef struct
{
  /* data */
  NSSInitContext* (*nssinit)(const char* configdir,
                             const char* certPrefix,
                             const char* keyPrefix,
                             const char* secmodName,
                             NSSInitParameters* initParams,
                             PRUint32 flags);

  PRBool (*nssisinit)();

  SECStatus (*nssshutdown)(NSSInitContext* context);
} nss_struct;

/*
nssinit = NSS_InitContext;
nssisint = NSS_IsInitialized;
nssshutdown = NSS_ShutdownContext;
*/

// Function for assigning the nss callbacks
void
nss_callbk_init(nss_struct* nss_pointer);

#endif /* __CALLBK_H_ */