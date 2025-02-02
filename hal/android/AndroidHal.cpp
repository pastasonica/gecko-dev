/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Hal.h"
#include "HalImpl.h"
#include "WindowIdentifier.h"
#include "AndroidBridge.h"
#include "mozilla/dom/network/Constants.h"
#include "mozilla/java/GeckoAppShellWrappers.h"
#include "mozilla/java/GeckoRuntimeWrappers.h"
#include "mozilla/widget/ScreenManager.h"
#include "nsPIDOMWindow.h"

using namespace mozilla::dom;
using namespace mozilla::hal;

namespace mozilla::hal_impl {

void Vibrate(const nsTArray<uint32_t>& pattern, WindowIdentifier&&) {
  // Ignore the WindowIdentifier parameter; it's here only because hal::Vibrate,
  // hal_sandbox::Vibrate, and hal_impl::Vibrate all must have the same
  // signature.

  // Strangely enough, the Android Java API seems to treat vibrate([0]) as a
  // nop.  But we want to treat vibrate([0]) like CancelVibrate!  (Note that we
  // also need to treat vibrate([]) as a call to CancelVibrate.)
  bool allZero = true;
  for (uint32_t i = 0; i < pattern.Length(); i++) {
    if (pattern[i] != 0) {
      allZero = false;
      break;
    }
  }

  if (allZero) {
    hal_impl::CancelVibrate(WindowIdentifier());
    return;
  }

  AndroidBridge* b = AndroidBridge::Bridge();
  if (!b) {
    return;
  }

  b->Vibrate(pattern);
}

void CancelVibrate(WindowIdentifier&&) {
  // Ignore WindowIdentifier parameter.

  java::GeckoAppShell::CancelVibrate();
}

void EnableBatteryNotifications() {
  java::GeckoAppShell::EnableBatteryNotifications();
}

void DisableBatteryNotifications() {
  java::GeckoAppShell::DisableBatteryNotifications();
}

void GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo) {
  AndroidBridge::Bridge()->GetCurrentBatteryInformation(aBatteryInfo);
}

void EnableNetworkNotifications() {
  java::GeckoAppShell::EnableNetworkNotifications();
}

void DisableNetworkNotifications() {
  java::GeckoAppShell::DisableNetworkNotifications();
}

void GetCurrentNetworkInformation(hal::NetworkInformation* aNetworkInfo) {
  AndroidBridge::Bridge()->GetCurrentNetworkInformation(aNetworkInfo);
}

void EnableScreenConfigurationNotifications() {
  java::GeckoAppShell::EnableScreenOrientationNotifications();
}

void DisableScreenConfigurationNotifications() {
  java::GeckoAppShell::DisableScreenOrientationNotifications();
}

void GetCurrentScreenConfiguration(ScreenConfiguration* aScreenConfiguration) {
  AndroidBridge* bridge = AndroidBridge::Bridge();
  if (!bridge) {
    return;
  }

  RefPtr<widget::Screen> screen =
      widget::ScreenManager::GetSingleton().GetPrimaryScreen();
  *aScreenConfiguration = screen->ToScreenConfiguration();
  aScreenConfiguration->orientation() =
      static_cast<hal::ScreenOrientation>(bridge->GetScreenOrientation());
  aScreenConfiguration->angle() = bridge->GetScreenAngle();
}

static bool IsSupportedScreenOrientation(hal::ScreenOrientation aOrientation) {
  // The Android backend only supports these orientations.
  static constexpr ScreenOrientation kSupportedOrientations[] = {
      ScreenOrientation::PortraitPrimary,
      ScreenOrientation::PortraitSecondary,
      ScreenOrientation::PortraitPrimary | ScreenOrientation::PortraitSecondary,
      ScreenOrientation::LandscapePrimary,
      ScreenOrientation::LandscapeSecondary,
      ScreenOrientation::LandscapePrimary |
          ScreenOrientation::LandscapeSecondary,
      ScreenOrientation::PortraitPrimary |
          ScreenOrientation::PortraitSecondary |
          ScreenOrientation::LandscapePrimary |
          ScreenOrientation::LandscapeSecondary,
      ScreenOrientation::Default,
  };
  for (auto supportedOrientation : kSupportedOrientations) {
    if (aOrientation == supportedOrientation) {
      return true;
    }
  }
  return false;
}

RefPtr<MozPromise<bool, bool, false>> LockScreenOrientation(
    const hal::ScreenOrientation& aOrientation) {
  using LockPromise = MozPromise<bool, bool, false>;

  if (!IsSupportedScreenOrientation(aOrientation)) {
    NS_WARNING("Unsupported screen orientation type");
    return LockPromise::CreateAndReject(false, __func__);
  }

  java::GeckoRuntime::LocalRef runtime = java::GeckoRuntime::GetInstance();
  if (!runtime) {
    return LockPromise::CreateAndReject(false, __func__);
  }
  auto result = runtime->LockScreenOrientation(uint32_t(aOrientation));
  auto geckoResult = java::GeckoResult::LocalRef(std::move(result));
  if (!geckoResult) {
    return LockPromise::CreateAndReject(false, __func__);
  }
  return LockPromise::FromGeckoResult(geckoResult);
}

void UnlockScreenOrientation() {
  java::GeckoRuntime::LocalRef runtime = java::GeckoRuntime::GetInstance();
  if (runtime) {
    runtime->UnlockScreenOrientation();
  }
}

}  // namespace mozilla::hal_impl
