#pragma once

#include <PcmData/PcmData.h>

namespace statemachine {

class IContext
{
public:
	static IContext* create(HWND hWnd, UINT msg);
	virtual ~IContext() {}

	class ICallback
	{
	public:
		virtual void onStarted(bool canPause) = 0;
		virtual void onStopped() = 0;
		virtual void onPaused() = 0;
		virtual void onResumed() = 0;
		virtual void onError(LPCTSTR source, HRESULT hr, LPCTSTR message) = 0;
	};

	virtual void setCallback(ICallback* callback) = 0;
	virtual ICallback* getCallback() = 0;

	// Methods to be called by Application.
	virtual HRESULT setup() = 0;
	virtual HRESULT shutdown() = 0;
	virtual HRESULT startTone(std::shared_ptr<IPcmData>& pcmData) = 0;
	virtual HRESULT startFile(LPCTSTR fileName) = 0;
	virtual HRESULT stop() = 0;
	virtual HRESULT pauseResume() = 0;
};
}
