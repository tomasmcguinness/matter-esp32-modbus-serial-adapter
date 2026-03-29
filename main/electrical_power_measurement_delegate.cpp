#include "electrical_power_measurement_delegate.h"
#include <app/reporting/reporting.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::DataModel;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ElectricalPowerMeasurement;
using namespace chip::app::Clusters::ElectricalPowerMeasurement::Structs;

static const MeasurementAccuracyRangeStruct::Type voltageAccuracyRanges[] = {
    {
        .rangeMin = 0,                                                               // 0V
        .rangeMax = 300'000,                                                         // 300V (in mV)
        .percentMax = chip::MakeOptional(static_cast<chip::Percent100ths>(1000)),    // 1%
        .percentMin = chip::MakeOptional(static_cast<chip::Percent100ths>(100)),     // 0.1%
        .percentTypical = chip::MakeOptional(static_cast<chip::Percent100ths>(500)), // 0.5%
    },
};

static const MeasurementAccuracyStruct::Type kMeasurementAccuracies[] = {
    {
        .measurementType = MeasurementTypeEnum::kVoltage,
        .measured = true,
        .minMeasuredValue = 0,
        .maxMeasuredValue = 300'000,
        .accuracyRanges = DataModel::List<const MeasurementAccuracyRangeStruct::Type>(voltageAccuracyRanges),
    },
};

CHIP_ERROR ElectricalPowerMeasurementDelegate::GetAccuracyByIndex(uint8_t index, MeasurementAccuracyStruct::Type &accuracy)
{
    if (index >= MATTER_ARRAY_SIZE(kMeasurementAccuracies))
    {
        return CHIP_ERROR_PROVIDER_LIST_EXHAUSTED;
    }
    accuracy = kMeasurementAccuracies[index];
    return CHIP_NO_ERROR;
}

CHIP_ERROR ElectricalPowerMeasurementDelegate::SetVoltage(DataModel::Nullable<int64_t> newValue)
{
    DataModel::Nullable<int64_t> oldValue = mVoltage;
    mVoltage = newValue;
    if (oldValue != newValue)
    {
        MatterReportingAttributeChangeCallback(mEndpointId, ElectricalPowerMeasurement::Id, Attributes::Voltage::Id);
    }
    return CHIP_NO_ERROR;
}
