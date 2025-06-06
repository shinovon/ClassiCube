# Compiling with Carbide.c++

## S60v3+

### Compatible SDKs
- Symbian^3 SDK v1.0
- Symbian Belle SDK v1.0
- SymbianSR1Qt474

### Compatible compilers
- GCCE 4.4.1
- RVCT 2.2 build 616
- RVCT 4.0 build 902 (only with SymbianSR1Qt474)

### Project setup
- File > Import... > Symbian OS > Symbian OS Bld.inf file
- Locate `misc\symbian\bld.inf`, select `SBSv2` builder, Next.
- Under `Nokia_Symbian3_SDK_v1.0`, select targets with `armv5_urel_gcce` and `armv5_urel` (second only if you have RVCT), Next.
- Select All, Next, Finish.
- Go to project properties, Carbide.c++>Build Configurations, select correct configuration
(e.g: `Phone Release (armv5_urel_gcce) [Nokia_Symbian3_SDK_v1.0]`).
- In SIS Builder tab, click Add.
- In PKG File drop down list, select `misc\symbian\ClassiCube.pkg`.
- Check `Don't sign sis file`, click OK.
- Apply and close properties.

## UIQ3

### Compatible SDKs
- UIQ 3.0 SDK

### Compatible compilers
- GCCE 3.4.3
- RVCT 2.2 build 616

### Project setup
- File > Import... > Symbian OS > Symbian OS Bld.inf file
- Locate `misc\symbian\bld.inf`, select `SBSv1` builder, Next.
- Select `UIQ3SDK`, Next.
- Select All, Next, Finish.
- Go to project properties, Carbide.c++>Build Configurations, select correct configuration
(e.g: `Phone Release (GCCE) [UIQ3SDK]`).
- In SIS Builder tab, click Add.
- In PKG File drop down list, select `misc\symbian\ClassiCube_UIQ3.pkg`.
- Check `Don't sign sis file`, click OK.
- Apply and close properties.
- Project > Build

(Based off comments from https://github.com/ClassiCube/ClassiCube/pull/1360)