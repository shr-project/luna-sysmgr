/* @@@LICENSE
*
* Copyright (c) 2013 Simon Busch
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#include <glib.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <pbnjson.hpp>
#include <map>
#include <set>
#include <QFile>
#include <QDebug>

#include "Common.h"
#include "ApplicationDescription.h"
#include "ApplicationManager.h"
#include "ApplicationDescription.h"
#include "AnimationSettings.h"
#include "HostBase.h"
#include "Logging.h"
#include "Settings.h"
#include "BootManager.h"
#include "Utils.h"
#include "WebAppMgrProxy.h"
#include "DisplayManager.h"

#include "cjson/json.h"
#include <pbnjson.hpp>
#include "JSONUtils.h"

static bool cbGetStatus(LSHandle *handle, LSMessage *message, void* user_data);

static LSMethod s_methods[]  = {
	{ "getStatus", cbGetStatus },
	{ 0, 0 },
};

void BootStateStartup::handleEvent(BootEvent event)
{
	if (event == BOOT_EVENT_WEBAPPMGR_AVAILABLE) {
		if (!QFile::exists("/var/luna/preferences/ran-first-use"))
			BootManager::instance()->switchState(BOOT_STATE_FIRSTUSE);
		else
			BootManager::instance()->switchState(BOOT_STATE_NORMAL);
	}
}

void BootStateFirstUse::enter()
{
	LSError error;
	LSErrorInit(&error);

	WebAppMgrProxy::instance()->launchBootTimeApp("org.webosports.app.firstuse");

	DisplayManager::instance()->pushDNAST("org.webosports.bootmgr");
}

void BootStateFirstUse::leave()
{
	// TODO close the firstuse application somehow or should it do this itself?

	DisplayManager::instance()->popDNAST("org.webosports.bootmgr");
}

void BootStateFirstUse::handleEvent(BootEvent event)
{
	if (event == BOOT_EVENT_FIRST_USE_DONE)
		createLocalAccount();
}

void BootStateFirstUse::createLocalAccount()
{
	LSError error;
	LSErrorInit(&error);

	QFile localProfileMarker("/var/luna/preferences/first-use-profile-created");
	if (!localProfileMarker.exists()) {
		if (!LSCall(BootManager::instance()->service(), "palm://com.palm.service.accounts/createLocalAccount",
					"{}", cbCreateLocalAccount, NULL, NULL, &error)) {
			g_warning("Failed to create local account after first use is done: %s", error.message);

			// regardless wether the local account creation was successfull or not we switch into
			// normal state
			BootManager::instance()->switchState(BOOT_STATE_NORMAL);

			return;
		}
	}
}

bool BootStateFirstUse::cbCreateLocalAccount(LSHandle *handle, LSMessage *message, void *user_data)
{
	// regardless wether the local account creation was successfull or not we switch into
	// normal state
	BootManager::instance()->switchState(BOOT_STATE_NORMAL);

	return true;
}

void BootStateNormal::enter()
{
	ApplicationManager* appMgr = ApplicationManager::instance();
	ApplicationDescription* sysUiAppDesc = appMgr ? appMgr->getAppById("com.palm.systemui") : 0;
	if (sysUiAppDesc) {
		std::string backgroundPath = "file://" + Settings::LunaSettings()->lunaSystemPath + "/index.html";
		WebAppMgrProxy::instance()->launchUrl(backgroundPath.c_str(), WindowType::Type_StatusBar,
		                 sysUiAppDesc, "", "{\"launchedAtBoot\":true}");
	}
	else {
		g_critical("Failed to launch System UI application");
	}

	ApplicationDescription* appDesc = appMgr ? appMgr->getAppById("com.palm.launcher") : 0;
	if (appDesc) {
		WebAppMgrProxy::instance()->launchUrl(appDesc->entryPoint().c_str(), WindowType::Type_Launcher,
						 appDesc, "", "{\"launchedAtBoot\":true}");
	}
	else {
		g_critical("Failed to launch Launcher application");
	}

	ApplicationManager::instance()->launchBootTimeApps();
}

void BootStateNormal::leave()
{
}

void BootStateNormal::handleEvent(BootEvent event)
{
}

BootManager* BootManager::instance()
{
	static BootManager *instance = 0;

    if (!instance)
		instance = new BootManager;

	return instance;
}

BootManager::BootManager() :
	m_service(0),
	m_webAppMgrAvailable(false),
	m_currentState(BOOT_STATE_STARTUP)
{
	m_states[BOOT_STATE_STARTUP] = new BootStateStartup();
	m_states[BOOT_STATE_FIRSTUSE] = new BootStateFirstUse();
	m_states[BOOT_STATE_NORMAL] = new BootStateNormal();

	startService();
	switchState(BOOT_STATE_STARTUP);

	m_fileWatch.addPath("/var/luna/preferences");
	connect(&m_fileWatch, SIGNAL(directoryChanged(QString)), this, SLOT(onFileChanged(QString)));
	connect(&m_fileWatch, SIGNAL(fileChanged(QString)), this, SLOT(onFileChanged(QString)));

	qDebug() << __PRETTY_FUNCTION__ << m_fileWatch.directories() << m_fileWatch.files();
}

BootManager::~BootManager()
{
	stopService();

	delete m_states[BOOT_STATE_STARTUP];
	delete m_states[BOOT_STATE_FIRSTUSE];
	delete m_states[BOOT_STATE_NORMAL];
}

void BootManager::startService()
{
	bool result;
	LSError error;
	LSErrorInit(&error);

	GMainLoop *mainLoop = HostBase::instance()->mainLoop();

	g_debug("BootManager starting...");

	if (!LSRegister("org.webosports.bootmgr", &m_service, &error)) {
		g_warning("Failed in BootManager: %s", error.message);
		LSErrorFree(&error);
		return;
	}

	if (!LSRegisterCategory(m_service, "/", s_methods, NULL, NULL, &error)) {
		g_warning("Failed in BootManager: %s", error.message);
		LSErrorFree(&error);
		return;
	}

	if (!LSGmainAttach(m_service, mainLoop, &error)) {
		g_warning("Failed in BootManager: %s", error.message);
		LSErrorFree(&error);
		return;
	}

	if (!LSCall(m_service, "palm://com.palm.lunabus/signal/registerServerStatus",
					"{\"serviceName\":\"org.webosports.webappmanager\"}",
					cbWebAppMgrAvailable, NULL, NULL, &error)) {
		g_warning("Failed in BootManager: %s", error.message);
		LSErrorFree(&error);
		return;
	}
}

void BootManager::stopService()
{
	LSError error;
	LSErrorInit(&error);
	bool result;

	result = LSUnregister(m_service, &error);
	if (!result)
		LSErrorFree(&error);

	m_service = 0;
}

void BootManager::switchState(BootState state)
{
	m_states[m_currentState]->leave();
	m_currentState = state;
	m_states[m_currentState]->enter();

	postCurrentState();
}

void BootManager::handleEvent(BootEvent event)
{
	m_states[m_currentState]->handleEvent(event);
}

void BootManager::onFileChanged(const QString& path)
{
	if (QFile::exists("/var/luna/preferences/ran-first-use"))
		handleEvent(BOOT_EVENT_FIRST_USE_DONE);
}

bool BootManager::cbWebAppMgrAvailable(LSHandle *handle, LSMessage *message, void *user_data)
{
	VALIDATE_SCHEMA_AND_RETURN(handle, message,
		SCHEMA_2(REQUIRED(serviceName, string), REQUIRED(connected, boolean)));

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (!json || is_error(json))
		return true;

	json_object* label = json_object_object_get(json, "connected");
	if (!label || !json_object_is_type(label, json_type_boolean)) {
		json_object_put(json);
		return true;
	}

	bool connected = json_object_get_boolean(label);
	BootManager::instance()->handleEvent(connected ? BOOT_EVENT_WEBAPPMGR_AVAILABLE : BOOT_EVENT_WEBAPPMGR_NOT_AVAILABLE);

	json_object_put(json);

	return true;
}

BootState BootManager::currentState() const
{
	return m_currentState;
}

LSHandle* BootManager::service() const
{
	return m_service;
}

std::string bootStateToStr(BootState state)
{
	std::string stateStr = "unknown";

	switch (state) {
	case BOOT_STATE_STARTUP:
		stateStr = "startup";
		break;
	case BOOT_STATE_FIRSTUSE:
		stateStr = "firstuse";
		break;
	case BOOT_STATE_NORMAL:
		stateStr = "normal";
		break;
	}

	return stateStr;
}

void BootManager::postCurrentState()
{
	LSError error;
	json_object* json = 0;

	LSErrorInit(&error);

	json = json_object_new_object();

	std::string stateStr = bootStateToStr(m_currentState);
	json_object_object_add(json, (char*) "state", json_object_new_string(stateStr.c_str()));

	if (!LSSubscriptionPost(m_service, "/", "getStatus", json_object_to_json_string(json), &error))
		LSErrorFree (&error);

	json_object_put(json);
}

bool cbGetStatus(LSHandle *handle, LSMessage *message, void *user_data)
{
	SUBSCRIBE_SCHEMA_RETURN(handle, message);

	bool success = true;
	LSError error;
	json_object* json = json_object_new_object();
	bool subscribed = false;
	bool firstUse = false;
	std::string stateStr;

	LSErrorInit(&error);

	if (LSMessageIsSubscription(message)) {
		if (!LSSubscriptionProcess(handle, message, &subscribed, &error)) {
			LSErrorFree (&error);
			goto done;
		}
	}

	stateStr = bootStateToStr(BootManager::instance()->currentState());
	json_object_object_add(json, "state", json_object_new_string(stateStr.c_str()));

done:
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));

	if (!LSMessageReply(handle, message, json_object_to_json_string(json), &error))
		LSErrorFree (&error);

	json_object_put(json);

	return true;
}