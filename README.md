# HookAndInject
Implementation of the well-known hook&inject method for intercepting calls to WinAPI functions inside a process without using the Windows Detours API: https://habr.com/ru/post/90377/

The project consists of the injector application and DLL. The application is responsible for injecting the DLL into the target process. Injecting requires acquiring debugging privileges. DLL once being injected into the process, starts to intercept specified function calls. Targeted function pointers with the realized stubs are listed in the Proxies.h/.cpp. JMP near instruction followed by the relative offset in the memory is used for the function call interceptions. During function substitution, all threads in the target process are stopped and then resumed.
