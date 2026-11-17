add_lldb_library(lldbPluginABIM88k PLUGIN
  ABISysV_m88k.cpp

  LINK_LIBS
    lldbCore
    lldbSymbol
    lldbTarget
    lldbValueObject
  LINK_COMPONENTS
    Support
    TargetParser
  )
