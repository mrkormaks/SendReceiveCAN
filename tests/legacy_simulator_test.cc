#include "sim/legacy_simulator.h"

#include <gtest/gtest.h>

#include <memory>

namespace can_bridge {
namespace {

class LegacySimulatorTest : public ::testing::Test {
 protected:
  // Value-initialized, like a freshly created file mapping.
  std::unique_ptr<SHAVN_410> avn_ = std::make_unique<SHAVN_410>();
  std::unique_ptr<SHSUT> sut_ = std::make_unique<SHSUT>();
  LegacySimulator simulator_{avn_.get(), sut_.get()};
};

TEST_F(LegacySimulatorTest, WritesLevelFieldsInLegacyLayout) {
  PublishBatch batch;
  batch.levels.mfi[kMfiRight].fk_buttons[12] = true;
  batch.levels.mfi[kMfiCenter].mfk_buttons[28] = true;
  batch.levels.mfi[kMfiLeft].brightness_step = -1;
  batch.levels.mfi[kMfiLeft].inner_knob_step = 1;
  batch.levels.mfi[kMfiRight].outer_knob_step = -1;
  batch.levels.mfi[kMfiCenter].indik_button = true;
  batch.levels.mfpu_buttons[kSideRight][73] = true;
  batch.levels.isrp.baro_press = true;
  batch.levels.isrp.fast_correction = true;
  batch.levels.isrp.baro_roll_steps = 5;
  batch.levels.isrp.hdg_roll_steps = -2;
  batch.levels.isrp.kontur_percent = 40;
  batch.levels.so2010.buttons[11] = true;
  batch.levels.so2010.entered_code_a = 7700;

  simulator_.Publish(batch);

  const SHMFI_IN& mfi = avn_->IBKO.MFI_IN;
  EXPECT_TRUE(mfi.Buttons_FK[12][kMfiRight]);
  EXPECT_FALSE(mfi.Buttons_FK[12][kMfiLeft]);
  EXPECT_TRUE(mfi.Buttons_MFK[28][kMfiCenter]);
  EXPECT_EQ(mfi.brightness[kMfiLeft], -1);
  EXPECT_EQ(mfi.indik_internal[kMfiLeft], 1);
  EXPECT_EQ(mfi.indik_external[kMfiRight], -1);
  EXPECT_TRUE(mfi.indik_button[kMfiCenter]);
  EXPECT_TRUE(avn_->IBKO.MFPU_IN.Buttons[73][kSideRight]);
  EXPECT_FALSE(avn_->IBKO.MFPU_IN.Buttons[73][kSideLeft]);

  EXPECT_TRUE(avn_->CAB.ISRP26.BARO_press);
  EXPECT_TRUE(avn_->COM.PRIB.ISRP26.BARO_press);
  EXPECT_TRUE(avn_->CAB.ISRP26.Fast_Korrection);
  EXPECT_DOUBLE_EQ(avn_->CAB.ISRP26.BARO_roll, 2.5);
  EXPECT_DOUBLE_EQ(avn_->COM.PRIB.ISRP26.BARO_roll, 2.5);
  EXPECT_DOUBLE_EQ(avn_->CAB.ISRP26.HDG_roll, -1.0);
  EXPECT_DOUBLE_EQ(avn_->CAB.Kontur1, 40.0);

  EXPECT_TRUE(avn_->CAN.fromCan.buttons[11]);
  EXPECT_EQ(avn_->CAN.fromCan.enteredCodeA, 7700);
}

TEST_F(LegacySimulatorTest, DirectWritesTouchOnlyTheirField) {
  avn_->IBKO.B8_LEFT_IN.button_1 = true;  // e.g. not yet consumed.
  avn_->IBKO.B8_RIGHT_IN.nabor_roll = 10.0;
  avn_->PCIE_1680_E1_1[3] = true;

  PublishBatch batch;
  batch.direct_writes = {
      AssignField(FieldKind::kB8Button, kSideLeft,
                  static_cast<int>(B8Button::kPsh2), 1),
      AssignField(FieldKind::kB8Button, kSideRight,
                  static_cast<int>(B8Button::kDigit9), 1),
      AssignField(FieldKind::kB8SelectorPosition, kSideRight,
                  static_cast<int>(B8Selector::kVolume), 5),
      AddToField(FieldKind::kB8SelectorRoll, kSideRight,
                 static_cast<int>(B8Selector::kSet), -1),
      AssignField(FieldKind::kOverheadSwitch, 0, 109, 1),
  };
  simulator_.Publish(batch);

  EXPECT_TRUE(avn_->IBKO.B8_LEFT_IN.button_psh_2);
  EXPECT_TRUE(avn_->IBKO.B8_LEFT_IN.button_1);  // Untouched.
  EXPECT_EQ(avn_->IBKO.B8_RIGHT_IN.button_9, 1);
  EXPECT_EQ(avn_->IBKO.B8_RIGHT_IN.gromk, 5);
  EXPECT_DOUBLE_EQ(avn_->IBKO.B8_RIGHT_IN.nabor_roll, 9.0);
  EXPECT_TRUE(avn_->PCIE_1680_E1_1[109]);
  EXPECT_TRUE(avn_->PCIE_1680_E1_1[3]);  // Untouched.
}

TEST_F(LegacySimulatorTest, ReadsIndicatorsAndControlFlags) {
  avn_->CAN.toCan.KompasKurs = 123.4;
  avn_->CAN.toCan.Sharik = -0.5;
  avn_->CAN.toCan.ark_Freq = 345.5;
  avn_->CAN.toCan.message = 3;
  avn_->CAN.toCan.displayedCode = 2000;

  const protocol::IndicatorValues values = simulator_.ReadIndicators();
  EXPECT_DOUBLE_EQ(values.compass_heading_deg, 123.4);
  EXPECT_DOUBLE_EQ(values.slip_ball, -0.5);
  EXPECT_DOUBLE_EQ(values.adf_frequency_khz, 345.5);
  EXPECT_EQ(values.so2010_message, 3);
  EXPECT_EQ(values.so2010_code, 2000);

  EXPECT_FALSE(simulator_.IsCloseRequested());
  sut_->Close = true;
  EXPECT_TRUE(simulator_.IsCloseRequested());

  simulator_.SetDeviceFault(2, true);
  EXPECT_TRUE(sut_->RMI_IN.DevOtk2);
  EXPECT_FALSE(sut_->RMI_IN.DevOtk1);
  simulator_.SetDeviceFault(2, false);
  EXPECT_FALSE(sut_->RMI_IN.DevOtk2);
  simulator_.SetDeviceFault(0, true);  // Disabled flag: no effect.
  EXPECT_FALSE(sut_->RMI_IN.DevOtk1);
}

}  // namespace
}  // namespace can_bridge
