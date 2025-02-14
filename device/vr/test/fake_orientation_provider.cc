// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/test/fake_orientation_provider.h"

#include "services/device/public/interfaces/sensor.mojom.h"

namespace device {

FakeOrientationSensor::FakeOrientationSensor(mojom::SensorRequest request)
    : binding_(this) {
  binding_.Bind(std::move(request));
}

FakeOrientationSensor::~FakeOrientationSensor() = default;

// The called functions
void FakeOrientationSensor::AddConfiguration(
    const PlatformSensorConfiguration& configuration,
    AddConfigurationCallback callback) {
  std::move(callback).Run(true);
}

}  // namespace device
