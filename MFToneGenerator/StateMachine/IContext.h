#pragma once

namespace statemachine {

class IContext
{
public:
	static IContext* create(HWND hWnd, UINT msg);
	virtual ~IContext() {}

	class ICallback
	{
	public:
		virtual void onStarted() = 0;
		virtual void onStopped() = 0;
		virtual void onPaused() = 0;
		virtual void onResumed() = 0;
		virtual void onError(LPCTSTR source, HRESULT hr, LPCTSTR message) = 0;
	};

	virtual void setCallback(ICallback* callback) = 0;
	virtual ICallback* getCallback() = 0;
	virtual void setAudioFileName(LPCTSTR value) = 0;
	virtual LPCTSTR getAudioFileName() const = 0;

	// Methods to be called by Application.
	virtual HRESULT setup() = 0;
	virtual HRESULT shutdown() = 0;
	virtual HRESULT setKey(float key) = 0;
	virtual HRESULT startStop() = 0;
	virtual HRESULT pauseResume() = 0;
};
}
