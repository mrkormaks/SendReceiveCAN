// STUB for tests and compile checks. NOT the real simulator layout.
// Declares only the SHSUT fields the bridge uses.
#ifndef CAN_BRIDGE_TESTS_STUB_STRUCT_SUT_410_H_
#define CAN_BRIDGE_TESTS_STUB_STRUCT_SUT_410_H_

struct SHRMI_IN {
  bool DevOtk1;
  bool DevOtk2;
};

struct SHSUT {
  SHRMI_IN RMI_IN;
  bool Close;
};

#endif  // CAN_BRIDGE_TESTS_STUB_STRUCT_SUT_410_H_
