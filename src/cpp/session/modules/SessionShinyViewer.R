#
# SessionShinyViewer.R
#
# Copyright (C) 2009-14 by RStudio, Inc.
#
# Unless you have received this program directly from RStudio pursuant
# to the terms of a commercial license agreement with RStudio, then
# this program is licensed to you under the terms of version 3 of the
# GNU Affero General Public License. This program is distributed WITHOUT
# ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
# MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
# AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
#
#

.rs.addFunction("invokeShinyWindowViewer", function(url) {
   invisible(.Call("rs_shinyviewer", url, getwd()))
})

.rs.addFunction("invokeShinyPaneViewer", function(url) {
   invisible(.Call("rs_viewer", url, NULL))
})

.rs.addFunction("setShinyViewerType", function(type) {
   if (type == 1)
      options(shiny.launch.browser = FALSE)
   else if (type == 2)
      options(shiny.launch.browser = .rs.invokeShinyPaneViewer)
   else if (type == 3)
      options(shiny.launch.browser = .rs.invokeShinyWindowViewer)
   else if (type == 4)
      options(shiny.launch.browser = TRUE)
})

.rs.addFunction("getShinyViewerType", function() {
   viewer <- getOption("shiny.launch.browser")
   if (identical(viewer, FALSE))
      return(1)
   else if (identical(viewer, .rs.invokeShinyPaneViewer))
      return(2)
   else if (identical(viewer, .rs.invokeShinyWindowViewer))
      return(3)
   else if (identical(viewer, TRUE))
      return(4)
   return(0)
})

.rs.addJsonRpcHandler("get_shiny_viewer_type", function() {
   list(viewerType = .rs.scalar(.rs.getShinyViewerType()))
})

