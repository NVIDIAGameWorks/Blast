//
// ApplicationView.cpp - from a basic VS2012 XBOXONE template
//

#include <windows.h>
#include <pix.h>
#if defined USE_PIX
#include <d3d11_x.h>
#endif
#include <gtest/gtest.h>

using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;

ref class ApplicationView sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:

    ApplicationView();

    // IFrameworkView Methods
    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
    virtual void Load(Platform::String^ entryPoint);
    virtual void Run();
    virtual void Uninitialize();

protected:

    // Event Handlers
    void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
    void OnResuming(Platform::Object^ sender, Platform::Object^ args);
    void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

private:

    bool m_windowClosed;
};

// ApplicationSource - responsible for creating the Application instance
// and passing it back to the system
ref class ApplicationViewSource : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};

ApplicationView::ApplicationView()
{
    m_windowClosed = false;
}

// Called by the system.  Perform application initialization here,
// hooking application wide events, etc.
void ApplicationView::Initialize(CoreApplicationView^ applicationView)
{
    applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &ApplicationView::OnActivated);
    CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &ApplicationView::OnSuspending);
    CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &ApplicationView::OnResuming);
}

// Called when we are provided a window.
void ApplicationView::SetWindow(CoreWindow^ window)
{
    window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &ApplicationView::OnWindowClosed);
}

// The purpose of this method is to get the application entry point.
void ApplicationView::Load(Platform::String^ entryPoint)
{
}

// Called by the system after initialization is complete.  This
// implements the traditional game loop
void ApplicationView::Run()
{
	CoreDispatcher^ dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;

	{
		dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

		freopen("stderr.txt", "wt", stderr);
		freopen("stdout.txt", "wt", stdout);

		//fprintf(stderr,"hello err\n");
		//fprintf(stdout,"hello out\n");

#if defined(USE_PIX)
		{
			UINT creationFlags = 0; // D3D11_CREATE_DEVICE_INSTRUMENTED;
			D3D_FEATURE_LEVEL featureLevels = D3D_FEATURE_LEVEL_11_1;
			HRESULT created = D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_HARDWARE,
				nullptr,
				creationFlags,
				&featureLevels, 1,
				D3D11_SDK_VERSION,
				nullptr,
				nullptr,
				nullptr
			);

			if (FAILED(created))
			{
				std::cerr << "D3D11CreateDevice failed " << created;
				fflush(stderr);
				throw Platform::Exception::CreateException(created);
			}
		}
#endif

		int retcode = RUN_ALL_TESTS();

		fflush(stderr);
		fflush(stdout);

		fclose(stderr);
		fclose(stdout);
	}
}

void ApplicationView::Uninitialize()
{
}

// Called when the application is activated.
void ApplicationView::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    CoreWindow::GetForCurrentThread()->Activate();

	if (args->Kind == ActivationKind::Launch)
	{
		LaunchActivatedEventArgs ^launchArgs = (LaunchActivatedEventArgs ^) args;
		Platform::String^ a = launchArgs->Arguments;
		wchar_t string[1024];
		wcscpy_s(string,a->Data());
			
		wchar_t* argv[256];
		argv[0] = L"SdkTestApp";
		int argc = 1;

		wchar_t * last = string;
		while(last)
		{
			wchar_t *pos = wcschr(last, ' ');
			if(pos == nullptr)
			{
				if(*last !='\0')
					argv[argc++] = last;
				
				break;
			}

			*pos = '\0';
			if(*last !='\0')
				argv[argc++] = last;
			last = pos + 1;

		}
	
		::testing::UnitTest::GetInstance()->listeners().Append(new CodeCoverageListener);
		testing::InitGoogleTest(&argc, argv);
	}
}

// Called when the application is suspending.
void ApplicationView::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
    // TODO: Save game progress using the ConnectedStroage API.
}

// Called when the application is resuming from suspended.
void ApplicationView::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
    // TODO: Handle changes in users and input devices.
}

void ApplicationView::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
    m_windowClosed = true;
}

// Implements a IFrameworkView factory.
IFrameworkView^ ApplicationViewSource::CreateView()
{
    return ref new ApplicationView();
}

//// Application entry point
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    auto applicationViewSource = ref new ApplicationViewSource();

    CoreApplication::Run(applicationViewSource);

    return 0;
}
