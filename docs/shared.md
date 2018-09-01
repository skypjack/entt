### EnTT and shared libraries

To make sure that an application and a shared library that use both `EnTT` can
interact correctly when symbols are hidden by default, there are some tricks to
follow.<br/>
In particular and in order to avoid undefined behaviors, all the instantiation
of the `Family` class template shall be made explicit along with the system-wide
specifier to use to export them.

At the time I'm writing this document, the classes that use internally the above
mentioned class template are `Dispatcher`, `Emitter` and `Registry`. Therefore
and as an example, if you use the `Registry` class template in your shared
library and want to set symbols visibility to _hidden_ by default, the following
lines are required to allow it to function properly with a client that also uses
the `Registry` somehow:

* On GNU/Linux:

  ```cpp
  namespace entt {
      template class __attribute__((visibility("default"))) Family<struct InternalRegistryTagFamily>;
      template class __attribute__((visibility("default"))) Family<struct InternalRegistryComponentFamily>;
      template class __attribute__((visibility("default"))) Family<struct InternalRegistryHandlerFamily>;
  }
  ```

* On Windows:

  ```cpp
  namespace entt {
      template class __declspec(dllexport) Family<struct InternalRegistryTagFamily>;
      template class __declspec(dllexport) Family<struct InternalRegistryComponentFamily>;
      template class __declspec(dllexport) Family<struct InternalRegistryHandlerFamily>;
  }
  ```

Otherwise, the risk is that type identifiers are different between the shared
library and the application and this will prevent the whole thing from
functioning correctly for obvious reasons.
