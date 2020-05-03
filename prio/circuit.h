/*
 * Copyright (c) 2020, Daniel Reusche
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CIRCUIT_H__
#define __CIRCUIT_H__

#include "encode.h"
#include "server.h"
#include <mprio.h>

/*
 * For the b-bit integer circuit, the single bit circuit is used as a
 * building block. The polynomials are set s.t. the bits of every
 * EInt, serially, are the bits to be shared. For n EInts,
 * this bitfield contains n*b bits.
 *
 * The x of every EInt contains an accumulation of the (shares of the)
 * bits, to enable checking wether they correctly implement the (share
 * of the) integer x. Since this is an affine transformation it is
 * independent of the part of the circuit containing multiplication
 * gates.
 */

/*
 * Set points of f and g for client packages.
 */
SECStatus
IntCircuit_set_fg_client(const_PrioConfig cfg,
                         const_EIntArray data_in,
                         MPArray points_f,
                         MPArray points_g);

/*
 * Compute and set points of f and g shares for servers packages.
 */
SECStatus
IntCircuit_set_fg_server(PrioVerifier v,
                         const_EIntArray shares_in,
                         MPArray points_f,
                         MPArray points_g);

/*
 * Since this part of the circuit uses only affine transformations,
 * each server can execute it on shares only.
 */
SECStatus
IntCircuit_check_x_accum(const_PrioConfig cfg, const_EIntArray shares_in);

#endif /* __CIRCUIT_H__ */
