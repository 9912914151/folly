/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <folly/io/async/ssl/SSLErrors.h>

#include <folly/Range.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

using namespace folly;

namespace {

std::string decodeOpenSSLError(
    int sslError,
    unsigned long errError,
    int sslOperationReturnValue) {
  if (sslError == SSL_ERROR_SYSCALL && errError == 0) {
    if (sslOperationReturnValue == 0) {
      return "SSL_ERROR_SYSCALL: EOF";
    } else {
      // In this case errno is set, AsyncSocketException will add it.
      return "SSL_ERROR_SYSCALL";
    }
  } else if (sslError == SSL_ERROR_ZERO_RETURN) {
    // This signifies a TLS closure alert.
    return "SSL_ERROR_ZERO_RETURN";
  } else {
    std::array<char, 256> buf;
    std::string msg(ERR_error_string(errError, buf.data()));
    return msg;
  }
}

const StringPiece getSSLErrorString(SSLError error) {
  StringPiece ret;
  switch (error) {
    case SSLError::CLIENT_RENEGOTIATION:
      ret = "Client tried to renegotiate with server";
      break;
    case SSLError::INVALID_RENEGOTIATION:
      ret = "Attempt to start renegotiation, but unsupported";
      break;
    case SSLError::EARLY_WRITE:
      ret = "Attempt to write before SSL connection established";
      break;
    case SSLError::OPENSSL_ERR:
      // decodeOpenSSLError should be used for this type.
      ret = "OPENSSL error";
      break;
  }
  return ret;
}
}

namespace folly {

SSLException::SSLException(
    int sslError,
    unsigned long errError,
    int sslOperationReturnValue,
    int errno_copy)
    : AsyncSocketException(
          AsyncSocketException::SSL_ERROR,
          decodeOpenSSLError(sslError, errError, sslOperationReturnValue),
          sslError == SSL_ERROR_SYSCALL ? errno_copy : 0),
      sslError(SSLError::OPENSSL_ERR),
      opensslSSLError(sslError),
      opensslErr(errError) {}

SSLException::SSLException(SSLError error)
    : AsyncSocketException(
          AsyncSocketException::SSL_ERROR,
          getSSLErrorString(error).str(),
          0),
      sslError(error) {}
}
