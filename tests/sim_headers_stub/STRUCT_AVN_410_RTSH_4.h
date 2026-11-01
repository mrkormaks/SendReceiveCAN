// STUB for tests and compile checks. NOT the real simulator layout.
//
// Declares only the SHAVN_410 fields the bridge uses, with plausible types.
// The left and right B8 panels deliberately use different types and some
// fields use int/double instead of bool, to prove that the adapter does not
// depend on those details. Builds against this file refuse to run.
#ifndef CAN_BRIDGE_TESTS_STUB_STRUCT_AVN_410_RTSH_4_H_
#define CAN_BRIDGE_TESTS_STUB_STRUCT_AVN_410_RTSH_4_H_

#define CAN_BRIDGE_STUB_SIM_HEADERS 1

struct SHB8_LEFT_IN {
  bool line1treug, line2treug, line3treug;
  bool button_psh_1, button_psh_2, button_psh_3;
  bool button_APr, button_RI, button_sbros, button_vvod, button_CA;
  bool button_0, button_1, button_2, button_3, button_4;
  bool button_5, button_6, button_7, button_8, button_9;
  int nabor, gromk;
  int nabor_roll, gromk_roll;
};

struct SHB8_RIGHT_IN {
  int line1treug, line2treug, line3treug;
  int button_psh_1, button_psh_2, button_psh_3;
  int button_APr, button_RI, button_sbros, button_vvod, button_CA;
  int button_0, button_1, button_2, button_3, button_4;
  int button_5, button_6, button_7, button_8, button_9;
  short nabor, gromk;
  double nabor_roll, gromk_roll;
};

struct SHMFI_IN {
  bool Buttons_FK[13][3];
  bool Buttons_MFK[29][3];
  int brightness[3];
  bool brightness_button[3];
  int indik_internal[3];
  int indik_external[3];
  bool indik_button[3];
};

struct SHMFPU_IN {
  bool Buttons[74][2];
};

struct SHIBKO {
  SHB8_LEFT_IN B8_LEFT_IN;
  SHB8_RIGHT_IN B8_RIGHT_IN;
  SHMFI_IN MFI_IN;
  SHMFPU_IN MFPU_IN;
};

struct SHISRP26 {
  bool Brightness_minus, Brightness_plus, Fast_Korrection;
  bool HDG_press, BARO_press;
  double BARO_roll, HDG_roll;
};

struct SHCAB {
  SHISRP26 ISRP26;
  double Kontur1;
};

struct SHPRIB {
  SHISRP26 ISRP26;
};

struct SHCOM {
  SHPRIB PRIB;
};

struct SHCAN_TO {
  double KompasKurs;
  double Sharik;
  double ark_Freq;
  int message;
  int displayedCode;
};

struct SHCAN_FROM {
  bool buttons[12];
  int enteredCodeA;
};

struct SHCAN {
  SHCAN_TO toCan;
  SHCAN_FROM fromCan;
};

struct SHAVN_410 {
  SHCAN CAN;
  SHIBKO IBKO;
  SHCAB CAB;
  SHCOM COM;
  bool PCIE_1680_E1_1[128];
};

#endif  // CAN_BRIDGE_TESTS_STUB_STRUCT_AVN_410_RTSH_4_H_
