
// From https://github.com/UE4SS-RE/RE-UE4SS/issues/427
// TODO: Implement const version of 'GetPropertyByNameInChain' and all of the functions it depends on in the Unreal submodule.
//       After that, uncomment the const version of the getter function.
//       This is to allow getters to be used in const functions.
//       Despite the fact that we could allow mutations of properties in const functions,
//       we probably shouldn't (by making return types const) because C++ normally doesn't allow that.
// Helper macro to create a helper function for retrieving properties from class instances by name.
// Use this macro to define properties for classes that haven't been generated with UVTD, for example in C++ mods.
// Properties defined this way can be accessed by name without needing to worry about changes to the class layout brought by game or engine updates.
// USAGE EXAMPLE (in class): DEFINE_PROPERTY(UObject*, Mesh1P);
// USAGE EXAMPLE (access):   Output::send(STR("Mesh1P: {}\n"), MyInstance->GetMesh1P()->GetFullName());
// USAGE EXAMPLE (in class): DEFINE_PROPERTY(FName, Property With Space In Name, PropertyWithSpaceInName);
// USAGE EXAMPLE (access):   Output::send(STR("Mesh1P: {}\n"), MyInstance->GetPropertyWithSpaceInName().ToString());
// Implementation detail: We're not using the convenience API (GetValuePtrByPropertyNameInChain) for this because we don't know if the property always exists.
//                        If it does exist, we can retrieve the offset for the property, and the offset should never change at runtime so we can cache it.
#define RC_SELECT_ONE(One) Get##One
#define RC_SELECT_TWO(One, Two) Get##Two
#define RC_GET_SELECT_TWO_IF_EXISTS_MACRO(_1, _2, Name, ...) Name
#define RC_SELECT_TWO_IF_EXISTS(...) RC_GET_SELECT_TWO_IF_EXISTS_MACRO(__VA_ARGS__, RC_SELECT_TWO, RC_SELECT_ONE)(__VA_ARGS__)
#define RC_DEFINE_PROPERTY_BODY(Type, Name)                                                                                                                    \
    static const auto [Offset, ElementSize] = [&]() -> std::pair<int32, int32> {                                                                               \
        const auto Property = GetPropertyByNameInChain(STR(#Name));                                                                                            \
        if (!Property)                                                                                                                                         \
        {                                                                                                                                                      \
            throw std::runtime_error{std::format("ERROR: Property '" #Name "' not found in class '{}'\n", to_string(GetClassPrivate()->GetFullName()))};       \
        }                                                                                                                                                      \
        return {Property->GetOffset_Internal(), Property->GetSize()};                                                                                          \
    }();                                                                                                                                                       \
    auto ValuePtr = std::bit_cast<Type*>(std::bit_cast<uint8*>(this) + Offset + ElementSize);                                                                  \
    return *ValuePtr;
#define DEFINE_PROPERTY(Type, Name, ...)                                                                                                                       \
    inline Type& RC_SELECT_TWO_IF_EXISTS(Name __VA_OPT__(, ) __VA_ARGS__)()                                                                                    \
    {                                                                                                                                                          \
        RC_DEFINE_PROPERTY_BODY(Type, Name)                                                                                                                    \
    } /*                                                                                                                                                       \
     inline const Type& RC_SELECT_TWO_IF_EXISTS(Name __VA_OPT__(,) __VA_ARGS__)() const                                                                        \
     {                                                                                                                                                         \
         RC_DEFINE_PROPERTY_BODY(const Type, Name)                                                                                                             \
     }*/
