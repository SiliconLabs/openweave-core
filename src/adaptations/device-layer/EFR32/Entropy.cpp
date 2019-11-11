/*
 *
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Provides implementations for the Weave entropy sourcing functions
 *          on the Silcon Labs EFR32 platforms.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Support/crypto/WeaveRNG.h>
#include <openthread/random_crypto.h>
#include <mbedtls/entropy_poll.h>

using namespace ::nl;
using namespace ::nl::Weave;

#if !WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG
#error "Nest DRBG implementation must be enabled on EFR32 platforms"
#endif // !WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

#define WEAVE_ENTROPY_GEN_FAILED    1

extern "C"  int mbedtls_hardware_poll( void *data,
                           unsigned char *output, size_t len, size_t *olen );

/**
 * Retrieve entropy from the underlying RNG source.
 *
 * This function is called by the Nest DRBG to acquire entropy.
 */
int GetEntropy_EFR32(uint8_t * buf, size_t count)
{
    int res = 0;
    size_t entropy_len = 0;
    size_t olen = 0;
    
    VerifyOrDie(count <= UINT16_MAX);
    
    while (entropy_len < count)
    {
        res = mbedtls_hardware_poll( NULL, 
                                     buf + entropy_len,
                                     count - entropy_len, 
                                     &olen );
        if (res != 0)
        {
            return res;
        }

        entropy_len += olen;
    }

    if (entropy_len < count)
    {
        res = WEAVE_ENTROPY_GEN_FAILED;
    }

    return res;
}

WEAVE_ERROR InitEntropy()
{
    WEAVE_ERROR err;

    // Initialize the Nest DRBG.
    err = Platform::Security::InitSecureRandomDataSource(GetEntropy_EFR32, 64, NULL, 0);
    SuccessOrExit(err);

    // Seed the standard rand() pseudo-random generator with data from the secure random source.
    {
        unsigned int seed;
        err = Platform::Security::GetSecureRandomData((uint8_t *)&seed, sizeof(seed));
        SuccessOrExit(err);
        srand(seed);
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(Crypto, "InitEntropy() failed: 0x%08" PRIX32, err);
    }
    return err;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
