/*
 * RSessionLaunchProfile.hpp
 *
 * Copyright (C) 2009-12 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#ifndef CORE_R_UTIL_R_SESSION_LAUNCH_PROFILE_HPP
#define CORE_R_UTIL_R_SESSION_LAUNCH_PROFILE_HPP

#include <string>

#include <core/system/PosixSystem.hpp>

#include <core/json/Json.hpp>

namespace core {
namespace r_util {

struct SessionLaunchProfile
{
   std::string username;
   std::string executablePath;
   core::system::ProcessConfig config;
};

json::Object sessionLaunchProfileToJson(const SessionLaunchProfile& profile);

SessionLaunchProfile sessionLaunchProfileFromJson(
                                           const json::Object& jsonProfile);

} // namespace r_util
} // namespace core 


#endif // CORE_R_UTIL_R_SESSION_LAUNCH_PROFILE_HPP
