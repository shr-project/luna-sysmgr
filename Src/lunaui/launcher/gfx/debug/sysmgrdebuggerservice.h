/* @@@LICENSE
*
*      Copyright (c) 2010-2013 LG Electronics, Inc.
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




#ifndef SYSMGRDEBUGGERSERVICE_H_
#define SYSMGRDEBUGGERSERVICE_H_

#include <QObject>
#include <QPointer>
#include <QtGlobal>

class LSHandle;
class LSMessage;
class LSPalmService;
class PixPagerDebugger;

class SysmgrDebuggerService : public QObject
{
public:

	static SysmgrDebuggerService * service();
	~SysmgrDebuggerService();

	friend bool _createPixPagerDebugger(LSHandle* lshandle,LSMessage *message, void *user_data);

private:

	static QPointer<SysmgrDebuggerService> s_qp_instance;
	LSPalmService *	m_p_service;
	QPointer<PixPagerDebugger> m_qp_pixPagerDebugger;

	bool startup();
	bool shutdown();

	SysmgrDebuggerService() : m_p_service(0) {}
	SysmgrDebuggerService(const SysmgrDebuggerService& c) {}
	SysmgrDebuggerService& operator=(const SysmgrDebuggerService& c) { return *this; }

	bool createPixPagerDebugger(LSHandle* p_lshandle,LSMessage *p_message, void * p_userdata);

};

#endif /* SYSMGRDEBUGGERSERVICE_H_ */
