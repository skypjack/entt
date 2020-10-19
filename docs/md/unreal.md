# Using EnTT in Unreal Engine 4

## Enabling C++17
As of writing (Unreal Engine v4.25), the default C++ standard of Unreal Engine is C++14.
If your Unreal project already uses C++17, you can continue to the following section.

In your main Module, you should have a ```<Game Name>.Build.cs``` file. In the constructor, add the following lines:
```cs
PCHUsage = PCHUsageMode.NoSharedPCHs;
PrivatePCHHeaderFile = "<PCH filename>.h";
CppStandard = CppStandardVersion.Cpp17;
```

Replace ```<PCH filename>.h``` by the name of your PCH header file if you already have one.
If you don't, you can create one with this content:
```cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

```
Remove any old ```PCHUsage = <...>``` line that was previously there.

Try to compile your project to ensure it works in C++17 before following further steps.

**Note:** Updating your *project* to C++17 does not necessarily mean your *IDE* will start to recognize C++17
syntax. If you plan on using C++17 features, you should lookup specific instructions for the IDE you use.

## EnTT as a Third Party Module
After this section, your ```Source``` directory should look like this:
```
Source
|   MyGame.Target.cs
|   MyGameEditor.Target.cs
|
+---MyGame
|   |   MyGame.Build.cs
|   |   MyGame.h (PCH Header file)
|
\---ThirdParty
    \---EnTT
        |   EnTT.Build.cs
        |
        \---entt (GitHub repository content inside)
```
Firstly, create in ```Source``` the folder ```ThirdParty``` if it does not already exist. 
Inside ```ThirdParty```, create an ```EnTT``` folder.

In ```EnTT``` create a new file ```EnTT.Build.cs``` with the following content:
```cs
using System.IO;
using UnrealBuildTool;

public class EnTT : ModuleRules
{
    public EnTT(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        // Add any include paths for the plugin
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "entt", "src", "entt"));
    }
}
```
The last line indicates that the actual header files will be in ```EnTT/entt/src/entt```.

Download the repository of EnTT and place the folder next to ```EnTT.Build.cs```.

Finally, go back into the ```<Game Name>.Build.cs``` of your Game Module,
 add EnTT as a dependency at the end of the list:
```
PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", [...], "EnTT"
        });
```
**Note:** Your IDE might require a restart to start recognizing the new module for code-highlighting features and such.

## Include EnTT
In any source file of your project, add `#include "entt.hpp"` to use EnTT.

Try to create a registry with ```entt::registry Registry;``` to make sure everything compiles.
