// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_CTAP_GET_ASSERTION_REQUEST_PARAM_H_
#define DEVICE_FIDO_CTAP_GET_ASSERTION_REQUEST_PARAM_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/optional.h"
#include "device/fido/ctap_request_param.h"
#include "device/fido/public_key_credential_descriptor.h"

namespace device {

// Object that encapsulates request parameters for AuthenticatorGetAssertion as
// specified in the CTAP spec.
class CTAPGetAssertionRequestParam : public CTAPRequestParam {
 public:
  CTAPGetAssertionRequestParam(std::string rp_id,
                               std::vector<uint8_t> client_data_hash);
  CTAPGetAssertionRequestParam(CTAPGetAssertionRequestParam&& that);
  CTAPGetAssertionRequestParam& operator=(CTAPGetAssertionRequestParam&& other);
  ~CTAPGetAssertionRequestParam() override;

  base::Optional<std::vector<uint8_t>> Encode() const override;
  CTAPGetAssertionRequestParam& SetUserVerificationRequired(
      bool user_verfication_required);
  CTAPGetAssertionRequestParam& SetUserPresenceRequired(
      bool user_presence_required);
  CTAPGetAssertionRequestParam& SetAllowList(
      std::vector<PublicKeyCredentialDescriptor> allow_list);
  CTAPGetAssertionRequestParam& SetPinAuth(std::vector<uint8_t> pin_auth);
  CTAPGetAssertionRequestParam& SetPinProtocol(uint8_t pin_protocol);

 private:
  std::string rp_id_;
  std::vector<uint8_t> client_data_hash_;
  bool user_verification_required_ = false;
  bool user_presence_required_ = true;

  base::Optional<std::vector<PublicKeyCredentialDescriptor>> allow_list_;
  base::Optional<std::vector<uint8_t>> pin_auth_;
  base::Optional<uint8_t> pin_protocol_;

  DISALLOW_COPY_AND_ASSIGN(CTAPGetAssertionRequestParam);
};

}  // namespace device

#endif  // DEVICE_FIDO_CTAP_GET_ASSERTION_REQUEST_PARAM_H_
