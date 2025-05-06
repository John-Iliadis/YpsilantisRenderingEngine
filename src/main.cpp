#include "app/application.hpp"

int main()
{
    try
    {
        Application application;
        application.run();
    }
    catch (const std::exception& unhandledException)
    {
        MSGBOXPARAMSA mbp {};
        mbp.cbSize = sizeof(MSGBOXPARAMSA);
        mbp.hwndOwner = nullptr;
        mbp.hInstance = nullptr;
        mbp.lpszText = unhandledException.what();
        mbp.lpszCaption = "Unhandled Exception";
        mbp.dwStyle = MB_OK | MB_ICONINFORMATION;

        MessageBoxIndirectA(&mbp);
    }
}
