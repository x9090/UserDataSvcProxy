# UserDataSvcProxy
A demo Windows Service proxier. It acts as a proxy to Windows UserDataService and will attempt to adjust the default ESENT cache buffer size

## Usage
1. Build the project using solution file UserDataSvcProxy.sln
2. Currently, "ServiceDll" in UserDataSvcProxy.reg points to "c:\userdatasvc\UserDataSvcProxy.dll". Adjust accordingly if you place UserDataSvcProxy.dll in other location
3. Run UserDataSvcProxy.reg
4. To revert the changes, you can run UserDataSvc.reg instead