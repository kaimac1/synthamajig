/* my_debug.h
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DBG_PRINTF(fmt, ...) (void)0
#define EMSG_PRINTF(fmt, ...) (void)0
#define IMSG_PRINTF(fmt, ...) (void)0
#define myASSERT(e) ((void)0)



#ifdef __cplusplus
}
#endif

/* [] END OF FILE */
