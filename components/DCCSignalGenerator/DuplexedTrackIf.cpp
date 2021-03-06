/** \copyright
 * Copyright (c) 2014, Balazs Racz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are  permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \file LocalTrackIf.hxx
 *
 * Control flow that acts as a trackInterface and sends all packets to a local
 * fd that represents the DCC mainline, such as TivaDCC.
 *
 * NOTE: This has been customized for ESP32 Command Station to split the OPS
 * and PROG ioctl handling based on the send_long_preamble header flag. This
 * is not intended for merge back to OpenMRN.
 * 
 * @author Balazs Racz
 * @date 24 Aug 2014
 */

#include "can_ioctl.h"
#include "DuplexedTrackIf.h"

#include <dcc/Packet.hxx>
#include <executor/Executor.hxx>
#include <executor/StateFlow.hxx>
#include <sys/ioctl.h>
#include <utils/Buffer.hxx>
#include <utils/Queue.hxx>

namespace esp32cs
{

DuplexedTrackIf::DuplexedTrackIf(Service *service, int pool_size, int ops_fd
                               , int prog_fd)
    : StateFlow<Buffer<dcc::Packet>, QList<1>>(service)
    , fd_ops_(ops_fd), fd_prog_(prog_fd)
    , pool_(sizeof(Buffer<dcc::Packet>), pool_size)
{
}

StateFlowBase::Action DuplexedTrackIf::entry()
{
  auto *p = message()->data();
  auto fd = p->packet_header.send_long_preamble ?
    fd_prog_ : fd_ops_;
  HASSERT(fd >= 0);
  int ret = ::write(fd, p, sizeof(*p));
  if (ret < 0)
  {
    HASSERT(errno == ENOSPC);
    ::ioctl(fd, CAN_IOC_WRITE_ACTIVE, this);
    return wait();
  }
  return finish();
}

} // namespace esp32cs