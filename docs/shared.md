### EnTT and shared libraries

To make sure that an application and a shared library that use both `EnTT` can
interact correctly when symbols are hidden by default, there are some tricks to
follow.<br/>
In particular and in order to avoid undefined behaviors, all the instantiation
of the `family` class template shall be made explicit along with the system-wide
specifier to use to export them.

At the time I'm writing this document, the classes that use internally the above
mentioned class template are `dispatcher`, `emitter` and `registry`. Therefore
and as an example, if you use the `registry` class template in your shared
library and want to set symbols visibility to _hidden_ by default, the following
lines are required to allow it to function properly with a client that also uses
the `registry` somehow:

* On GNU/Linux:

  ```cpp
  namespace entt {
      template class __attribute__((visibility("default"))) family<struct internal_registry_component_family>;
      template class __attribute__((visibility("default"))) family<struct internal_registry_handler_family>;
  }
  ```

* On Windows:

  ```cpp
  namespace entt {
      template class __declspec(dllexport) family<struct internal_registry_component_family>;
      template class __declspec(dllexport) family<struct internal_registry_handler_family>;
  }
  ```

Otherwise, the risk is that type identifiers are different between the shared
library and the application and this will prevent the whole thing from
functioning correctly for obvious reasons.
