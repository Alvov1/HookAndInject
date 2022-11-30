# HookAndInject
Implementation of the well-known hook&inject method for intercepting calls to WinAPI functions inside a process without using the Windows Detours API.

Project is consisted ot the injector application and DLL. Application is responsible for injecting the DLL into the target process. Injecting process requires 
