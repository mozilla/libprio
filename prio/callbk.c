/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "callbk.h"
#include <nss.h>

void nss_callbk_init(nss_struct *nss_pointer){

    (nss_pointer)->nssinit = NSS_InitContext;
    (nss_pointer)->nssisinit = NSS_IsInitialized;
    (nss_pointer)->nssshutdown = NSS_ShutdownContext;
}