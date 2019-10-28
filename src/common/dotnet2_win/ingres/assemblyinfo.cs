using System.Reflection;
using System.Runtime.CompilerServices;
using System.Security;

//
// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
//
[assembly: AssemblyTitle      ("Ingres .NET Data Provider")]
[assembly: AssemblyDescription("Ingres .NET Data Provider")]
[assembly: AssemblyProduct    ("Ingres .NET Data Provider")]

[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("Actian Corporation")]
[assembly: AssemblyCopyright("� 2006, 2011 Actian Corporation. All Rights Reserved.")]
[assembly: AssemblyTrademark("")]
[assembly: AssemblyCulture("")]  // culture neutral
[assembly: System.Runtime.InteropServices.ComVisible(true)]
[assembly: System.CLSCompliant(true)]

//Allows strong-named assemblies to be called by partially trusted code.
//Without this declaration, only fully trusted callers would be able use
//the provider's assembly.  This attribute is new with .NET 1.1.
[assembly:AllowPartiallyTrustedCallers]

//
// Version information for an assembly consists of the following four values:
//
//      Major Version
//      Minor Version 
//      Build Number
//      Revision
//
// You can specify all the values or you can default the Revision and Build Numbers 
// by using the '*' as shown below:

[assembly: AssemblyVersion(    "2.1.0.0000")]
[assembly: AssemblyFileVersion("2.1.1010.42")]

//
// In order to sign your assembly you must specify a key to use. Refer to the 
// Microsoft .NET Framework documentation for more information on assembly signing.
//
// Use the attributes below to control which key is used for signing. 
//
// Notes: 
//   (*) If no key is specified, the assembly is not signed.
//   (*) KeyName refers to a key that has been installed in the Crypto Service
//       Provider (CSP) on your machine. KeyFile refers to a file which contains
//       a key.
//   (*) If the KeyFile and the KeyName values are both specified, the 
//       following processing occurs:
//       (1) If the KeyName can be found in the CSP, that key is used.
//       (2) If the KeyName does not exist and the KeyFile does exist, the key 
//           in the KeyFile is installed into the CSP and used.
//   (*) In order to create a KeyFile, you can use the sn.exe (Strong Name) utility.
//       When specifying the KeyFile, the location of the KeyFile should be
//       relative to the project output directory which is
//       %Project Directory%\obj\<configuration>. For example, if your KeyFile is
//       located in the project directory, you would specify the AssemblyKeyFile 
//       attribute as [assembly: AssemblyKeyFile("..\\..\\mykey.snk")]
//   (*) Delay Signing is an advanced option - see the Microsoft .NET Framework
//       documentation for more information on this.
//
//[assembly: AssemblyDelaySign(true)]
//[assembly: AssemblyKeyFile(@"..\..\..\specials\keypublic.snk")]

//[assembly: AssemblyDelaySign(false)]
//[assembly: AssemblyKeyFile(@"..\..\..\specials\keypair.snk")]

//[assembly: AssemblyDelaySign(false)]
//[assembly: AssemblyKeyFile("")]

//[assembly: AssemblyKeyName("")]
