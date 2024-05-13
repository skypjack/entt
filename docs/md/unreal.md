# EnTT and Unreal Engine

# Table of Contents

* [Enable Cpp17](#enable-cpp17)
* [EnTT as a third party module](#entt-as-a-third-party-module)
* [Include EnTT](#include-entt)

## Enable Cpp17

> Skip this part if you are working with UE5, Since UE5 uses cpp17 by default.

As of writing (Unreal Engine v4.25), the default C++ standard of Unreal Engine
is C++14.<br/>
On the other hand, note that `EnTT` requires C++17 to compile. To enable it, in
the main module of the project there should be a `<Game Name>.Build.cs` file,
the constructor of which must contain the following lines:

```cs
PCHUsage = PCHUsageMode.NoSharedPCHs;
PrivatePCHHeaderFile = "<PCH filename>.h";
CppStandard = CppStandardVersion.Cpp17;
```

Replace `<PCH filename>.h` with the name of the already existing PCH header
file, if any.<br/>
In case the project doesn't already contain a file of this type, it's possible
to create one with the following content:

```cpp
#pragma once
#include "CoreMinimal.h"
```

Remember to remove any old `PCHUsage = <...>` line that was previously there. At
this point, C++17 support should be in place.<br/>
Try to compile the project to ensure it works as expected before following
further steps.

Note that updating a *project* to C++17 doesn't necessarily mean that the IDE in
use will also start to recognize its syntax.<br/>
If the plan is to use C++17 in the project too, check the specific instructions
for the IDE in use.

## EnTT as a third party module

Once this point is reached, the `Source` directory should look like this:

```
Source
|  MyGame.Target.cs
|  MyGameEditor.Target.cs
|
+---MyGame
|  |  MyGame.Build.cs
|  |  MyGame.h (PCH Header file)
|
\---ThirdParty
   \---EnTT
      |   EnTT.Build.cs
      |
      \---entt (GitHub repository content inside)
```

To make this happen, create the folder `ThirdParty` under `Source` if it doesn't
exist already. Then, add an `EnTT` folder under `ThirdParty`.<br/>
Within the latter, create a new file `EnTT.Build.cs` with the following content:

```cs
using System.IO;
using UnrealBuildTool;

public class EnTT: ModuleRules {
    public EnTT(ReadOnlyTargetRules Target) : base(Target) {
        Type = ModuleType.External;
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "entt", "src", "entt"));
    }
}
```

The last line indicates that the actual files will be found in the directory
`EnTT/entt/src/entt`.<br/>
Download the repository for `EnTT` and place it next to `EnTT.Build.cs` or
update the path above accordingly.

Finally, open the `<Game Name>.Build.cs` file and add `EnTT` as a dependency at
the end of the list:

```cs
PublicDependencyModuleNames.AddRange(new[] {
    "Core", "CoreUObject", "Engine", "InputCore", [...], "EnTT"
});
```

Note that some IDEs might require a restart to start recognizing the new module
for code-highlighting features and such.

## Include EnTT

In any source file of the project, add `#include "entt.hpp"` or any other path
to the file from `EnTT` to use it.<br/>
Try to create a registry as `entt::registry registry;` to make sure everything
compiles fine.
