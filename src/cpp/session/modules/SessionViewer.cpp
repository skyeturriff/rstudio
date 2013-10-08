/*
 * SessionViewer.cpp
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

#include "SessionViewer.hpp"

#include <core/Error.hpp>
#include <core/Exec.hpp>

#include <r/RSexp.hpp>
#include <r/RRoutines.hpp>

#include <session/SessionModuleContext.hpp>

using namespace core;

namespace session {
namespace modules { 
namespace viewer {

namespace {

// track the current viewed url
std::string s_currentUrl;

// viewer stopped means clear the url
Error viewerStopped(const json::JsonRpcRequest& request,
                    json::JsonRpcResponse* pResponse)
{
   s_currentUrl.clear();
   return Success();
}


void viewerNavigate(const std::string& url, bool fullHeight = FALSE)
{
   // record the url (for reloads)
   s_currentUrl = url;

   // enque the event
   json::Object dataJson;
   dataJson["url"] = s_currentUrl;
   dataJson["full_height"] = fullHeight;
   ClientEvent event(client_events::kViewerNavigate, dataJson);
   module_context::enqueClientEvent(event);
}

// show error message from R
SEXP rs_browserInternal(SEXP urlSEXP, SEXP fullHeightSEXP)
{
   try
   {
      viewerNavigate(r::sexp::safeAsString(urlSEXP),
                     r::sexp::asLogical(fullHeightSEXP));
   }
   CATCH_UNEXPECTED_EXCEPTION

   return R_NilValue;
}

void onSuspend(const r::session::RSuspendOptions&, Settings*)
{
}

void onResume(const Settings&)
{
   viewerNavigate("about:blank");
}

void onClientInit()
{
   if (!s_currentUrl.empty())
      viewerNavigate(s_currentUrl);
}

} // anonymous namespace


Error initialize()
{
   // register rs_askForPassword with R
   R_CallMethodDef methodDefBrowserInternal ;
   methodDefBrowserInternal.name = "rs_browserInternal" ;
   methodDefBrowserInternal.fun = (DL_FUNC) rs_browserInternal ;
   methodDefBrowserInternal.numArgs = 2;
   r::routines::addCallMethod(methodDefBrowserInternal);

   // install event handlers
   using namespace module_context;
   events().onClientInit.connect(onClientInit);
   addSuspendHandler(SuspendHandler(onSuspend, onResume));

   // install rpc methods
   using boost::bind;
   ExecBlock initBlock ;
   initBlock.addFunctions()
      (bind(registerRpcMethod, "viewer_stopped", viewerStopped));
   return initBlock.execute();
}


} // namespace viewer
} // namespace modules
} // namesapce session

