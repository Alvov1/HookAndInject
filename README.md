# HookAndInject
Implementation of the well-known hook&inject method for intercepting calls to WinAPI functions inside a process without using the Windows Detours API.

Project is consisted ot the injector application and DLL. Application is responsible for injecting the DLL into the target process. Injecting requires acquiring the debugging priveleges. DLL once being injected into the process, starts to intercept specified function calls. Targetted function pointers with the realized stubs 
