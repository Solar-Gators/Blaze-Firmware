//
// Created by ahmed on 10/29/2026.
//

#ifndef BLAZEFIRMWARE_DAUGHTERBOARD_H
#define BLAZEFIRMWARE_DAUGHTERBOARD_H

#include "BQ76952.hpp"
#include "BQ769X2_Registers.h"

namespace bmsdaughter
{

void init();
void getCellVoltage();
void getCellTemp();
void sendBalancingCommand();
void sendStatusMessage();

}  // namespace bmsdaughter

#endif  //BLAZEFIRMWARE_DAUGHTERBOARD_H
