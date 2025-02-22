#pragma once

namespace DirectUI
{
    enum DUI_TIMER_REPEAT_OPTION
    {
        DTR_REPEAT = 0,
        DTR_NO_REPEAT = 1,
    };

    interface DECLSPEC_NOVTABLE IDUIActionInvokeHelper
    {
        virtual void DoInvoke() = 0;
    };

    class SimpleTimer
    {
    public:
        SimpleTimer(float flDuration, UINT cRepeats)
            : _hAction(nullptr)
            , _pInvoker(nullptr)
            , _flDuration(flDuration)
            , _cRepeats(cRepeats)
        {
        }

        SimpleTimer(float flDuration, DUI_TIMER_REPEAT_OPTION repeatOption)
            : SimpleTimer(flDuration, repeatOption == DTR_NO_REPEAT ? 0 : INT_MAX) // TODO INT_MAX assumed
        {
        }

        ~SimpleTimer()
        {
            Stop();
            delete _pInvoker;
        }

        void Start()
        {
            DelayedStart(_flDuration);
        }

        void DelayedStart(float flStartDelay)
        {
            Stop();

            GMA_ACTION ma = { sizeof(ma) };
            ma.flDelay = flStartDelay;
            ma.flDuration = 0.0;
            ma.flPeriod = _flDuration;
            ma.cRepeat = _cRepeats;
            ma.dwPause = -1;
            ma.pfnProc = s_ActionCallback;
            ma.pvData = this;

            _hAction = CreateAction(&ma);
        }

        bool Stop()
        {
            bool fStopped = true;

            if (_hAction)
            {
                fStopped = DeleteHandle(_hAction) != 0;
                _hAction = nullptr;
            }

            return fStopped;
        }

        bool IsRunning() const
        {
            return _hAction != nullptr;
        }

        float GetDuration() const
        {
            return _flDuration;
        }

        void SetDuration(float flDuration)
        {
            _flDuration = flDuration;
        }

        template <typename TFunc>
        class InvokeHandler final : public IDUIActionInvokeHelper
        {
        public:
            InvokeHandler(const TFunc& funcInvoke)
                : _funcInvoke(funcInvoke)
            {
            }

            InvokeHandler(const InvokeHandler&) = delete;

            void DoInvoke() override
            {
                _funcInvoke();
            }

        private:
            SimpleTimer& operator=(const InvokeHandler&) = delete;

            const TFunc _funcInvoke;
        };

        template <typename TFunc>
        HRESULT SetHandler(const TFunc& func)
        {
            delete _pInvoker;
            _pInvoker = new InvokeHandler<TFunc>(func);
            return _pInvoker ? S_OK : E_OUTOFMEMORY;
        }

    private:
        static void CALLBACK s_ActionCallback(GMA_ACTIONINFO* pActionInfo)
        {
            SimpleTimer* pTimer = (SimpleTimer*)pActionInfo->pvData;

            if (pActionInfo->fFinished)
            {
                pTimer->_hAction = nullptr;
            }
            else
            {
                pTimer->_pInvoker->DoInvoke();
            }
        }

        HACTION _hAction;
        IDUIActionInvokeHelper* _pInvoker;
        float _flDuration;
        UINT _cRepeats;
    };
}
