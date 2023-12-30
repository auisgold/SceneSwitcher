#include "macro-condition-filter.hpp"
#include "utility.hpp"

#include <regex>

namespace advss {

const std::string MacroConditionFilter::id = "filter";

bool MacroConditionFilter::_registered = MacroConditionFactory::Register(
	MacroConditionFilter::id,
	{MacroConditionFilter::Create, MacroConditionFilterEdit::Create,
	 "AdvSceneSwitcher.condition.filter"});

const static std::map<MacroConditionFilter::Condition, std::string>
	filterConditionTypes = {
		{MacroConditionFilter::Condition::ENABLED,
		 "AdvSceneSwitcher.condition.filter.type.active"},
		{MacroConditionFilter::Condition::DISABLED,
		 "AdvSceneSwitcher.condition.filter.type.showing"},
		{MacroConditionFilter::Condition::SETTINGS_MATCH,
		 "AdvSceneSwitcher.condition.filter.type.settingsMatch"},
		{MacroConditionFilter::Condition::SETTINGS_CHANGED,
		 "AdvSceneSwitcher.condition.filter.type.settingsChanged"},
		{MacroConditionFilter::Condition::INDIVIDUAL_SETTING_MATCH,
		 "AdvSceneSwitcher.condition.filter.type.individualSettingMatches"},
		{MacroConditionFilter::Condition::INDIVIDUAL_SETTING_CHANGED,
		 "AdvSceneSwitcher.condition.filter.type.individualSettingChanged"},
};

bool MacroConditionFilter::CheckConditionHelper(const OBSWeakSource &filter)
{
	bool ret = false;
	OBSSourceAutoRelease filterSource = obs_weak_source_get_source(filter);
	if (!filterSource.Get()) {
		return false;
	}

	switch (_condition) {
	case Condition::ENABLED:
		ret = obs_source_enabled(filterSource);
		break;
	case Condition::DISABLED:
		ret = !obs_source_enabled(filterSource);
		break;
	case Condition::SETTINGS_MATCH:
		ret = CompareSourceSettings(filter, _settings, _regex);
		if (IsReferencedInVars()) {
			SetVariableValue(GetSourceSettings(filter));
		}
		break;
	case Condition::SETTINGS_CHANGED: {
		std::string settings = GetSourceSettings(filter);
		ret = !_currentSettings.empty() && settings != _currentSettings;
		_currentSettings = settings;
		SetVariableValue(settings);
		break;
	}
	case Condition::INDIVIDUAL_SETTING_MATCH: {
		std::string value = GetSourceSettingValue(filter, _setting);
		ret = _regex.Enabled() ? _regex.Matches(value, _settings)
				       : value == std::string(_settings);
		SetVariableValue(value);
		break;
	}
	case Condition::INDIVIDUAL_SETTING_CHANGED: {
		std::string value = GetSourceSettingValue(filter, _setting);
		ret = _currentSettingsValue != value;
		_currentSettingsValue = value;
		SetVariableValue(value);
		break;
	}
	default:
		break;
	}
	return ret;
}

bool MacroConditionFilter::CheckCondition()
{
	auto filters = _filter.GetFilters(_source);
	if (filters.empty()) {
		return false;
	}

	bool ret = true;
	for (const auto &filter : filters) {
		ret = ret && CheckConditionHelper(filter);
	}

	if (GetVariableValue().empty()) {
		SetVariableValue(ret ? "true" : "false");
	}

	return ret;
}

bool MacroConditionFilter::Save(obs_data_t *obj) const
{
	MacroCondition::Save(obj);
	_source.Save(obj);
	_filter.Save(obj, "filter");
	obs_data_set_int(obj, "condition", static_cast<int>(_condition));
	_settings.Save(obj, "settings");
	_regex.Save(obj);
	_setting.Save(obj);
	return true;
}

bool MacroConditionFilter::Load(obs_data_t *obj)
{
	MacroCondition::Load(obj);
	_source.Load(obj);
	_filter.Load(obj, _source, "filter");
	_condition = static_cast<Condition>(obs_data_get_int(obj, "condition"));
	_settings.Load(obj, "settings");
	_regex.Load(obj);
	// TOOD: remove in future version
	if (obs_data_has_user_value(obj, "regex")) {
		_regex.CreateBackwardsCompatibleRegex(
			obs_data_get_bool(obj, "regex"));
	}
	_setting.Load(obj);
	return true;
}

std::string MacroConditionFilter::GetShortDesc() const
{
	if (!_filter.ToString().empty() && !_source.ToString().empty()) {
		return _source.ToString() + " - " + _filter.ToString();
	}
	return "";
}

static inline void populateConditionSelection(QComboBox *list)
{
	for (const auto &[_, name] : filterConditionTypes) {
		list->addItem(obs_module_text(name.c_str()));
	}
}

MacroConditionFilterEdit::MacroConditionFilterEdit(
	QWidget *parent, std::shared_ptr<MacroConditionFilter> entryData)
	: QWidget(parent),
	  _sources(new SourceSelectionWidget(this, QStringList(), true)),
	  _filters(new FilterSelectionWidget(this, _sources, true)),
	  _conditions(new QComboBox()),
	  _getSettings(new QPushButton(obs_module_text(
		  "AdvSceneSwitcher.condition.filter.getSettings"))),
	  _settings(new VariableTextEdit(this)),
	  _regex(new RegexConfigWidget(parent)),
	  _settingSelection(new SourceSettingSelection())
{
	populateConditionSelection(_conditions);
	auto sources = GetSourcesWithFilterNames();
	sources.sort();
	_sources->SetSourceNameList(sources);

	QWidget::connect(_sources,
			 SIGNAL(SourceChanged(const SourceSelection &)), this,
			 SLOT(SourceChanged(const SourceSelection &)));
	QWidget::connect(_filters,
			 SIGNAL(FilterChanged(const FilterSelection &)), this,
			 SLOT(FilterChanged(const FilterSelection &)));
	QWidget::connect(_conditions, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(ConditionChanged(int)));
	QWidget::connect(_getSettings, SIGNAL(clicked()), this,
			 SLOT(GetSettingsClicked()));
	QWidget::connect(_settings, SIGNAL(textChanged()), this,
			 SLOT(SettingsChanged()));
	QWidget::connect(_regex, SIGNAL(RegexConfigChanged(RegexConfig)), this,
			 SLOT(RegexChanged(RegexConfig)));
	QWidget::connect(_settingSelection,
			 SIGNAL(SelectionChanged(const SourceSetting &)), this,
			 SLOT(SettingSelectionChanged(const SourceSetting &)));

	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{sources}}", _sources},
		{"{{filters}}", _filters},
		{"{{conditions}}", _conditions},
		{"{{settings}}", _settings},
		{"{{getSettings}}", _getSettings},
		{"{{regex}}", _regex},
		{"{{settingSelection}}", _settingSelection},

	};
	auto line1Layout = new QHBoxLayout;
	line1Layout->setContentsMargins(0, 0, 0, 0);
	PlaceWidgets(obs_module_text(
			     "AdvSceneSwitcher.condition.filter.entry.line1"),
		     line1Layout, widgetPlaceholders);
	auto line2Layout = new QHBoxLayout;
	line2Layout->setContentsMargins(0, 0, 0, 0);
	PlaceWidgets(obs_module_text(
			     "AdvSceneSwitcher.condition.filter.entry.line2"),
		     line2Layout, widgetPlaceholders, false);
	auto line3Layout = new QHBoxLayout;
	line3Layout->setContentsMargins(0, 0, 0, 0);
	PlaceWidgets(obs_module_text(
			     "AdvSceneSwitcher.condition.filter.entry.line3"),
		     line3Layout, widgetPlaceholders);

	auto mainLayout = new QVBoxLayout;
	mainLayout->addLayout(line1Layout);
	mainLayout->addLayout(line2Layout);
	mainLayout->addLayout(line3Layout);
	setLayout(mainLayout);

	_entryData = entryData;
	UpdateEntryData();
	_loading = false;
}

void MacroConditionFilterEdit::SourceChanged(const SourceSelection &source)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_source = source;
}

void MacroConditionFilterEdit::FilterChanged(const FilterSelection &filter)
{
	if (_loading || !_entryData) {
		return;
	}

	{
		auto lock = LockContext();
		_entryData->_filter = filter;
	}
	const auto filters =
		_entryData->_filter.GetFilters(_entryData->_source);
	_settingSelection->SetSource(filters.empty() ? nullptr : filters.at(0));
	emit HeaderInfoChanged(
		QString::fromStdString(_entryData->GetShortDesc()));
}

void MacroConditionFilterEdit::ConditionChanged(int index)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_condition =
		static_cast<MacroConditionFilter::Condition>(index);
	SetWidgetVisibility();
}

void MacroConditionFilterEdit::GetSettingsClicked()
{
	if (_loading || !_entryData ||
	    _entryData->_filter.GetFilters(_entryData->_source).empty()) {
		return;
	}

	const auto filters =
		_entryData->_filter.GetFilters(_entryData->_source);
	if (filters.empty()) {
		_settings->setPlainText(QString(""));
		return;
	}

	QString value;
	if (_entryData->_condition ==
	    MacroConditionFilter::Condition::SETTINGS_MATCH) {
		value = FormatJsonString(GetSourceSettings(filters.at(0)));
	} else {
		value = QString::fromStdString(GetSourceSettingValue(
			filters.at(0), _entryData->_setting));
	}

	if (_entryData->_regex.Enabled()) {
		value = EscapeForRegex(value);
	}
	_settings->setPlainText(value);
}

void MacroConditionFilterEdit::SettingsChanged()
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_settings = _settings->toPlainText().toStdString();

	adjustSize();
	updateGeometry();
}

void MacroConditionFilterEdit::RegexChanged(RegexConfig conf)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_regex = conf;

	adjustSize();
	updateGeometry();
}

void MacroConditionFilterEdit::SettingSelectionChanged(
	const SourceSetting &setting)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_setting = setting;
}

void MacroConditionFilterEdit::SetWidgetVisibility()
{
	const bool showSettingsControls =
		_entryData->_condition ==
			MacroConditionFilter::Condition::SETTINGS_MATCH ||
		_entryData->_condition ==
			MacroConditionFilter::Condition::INDIVIDUAL_SETTING_MATCH;
	_settings->setVisible(showSettingsControls);
	_getSettings->setVisible(showSettingsControls);
	_regex->setVisible(showSettingsControls);
	_settingSelection->setVisible(
		_entryData->_condition ==
		MacroConditionFilter::Condition::INDIVIDUAL_SETTING_MATCH);
	adjustSize();
	updateGeometry();
}

void MacroConditionFilterEdit::UpdateEntryData()
{
	if (!_entryData) {
		return;
	}

	_sources->SetSource(_entryData->_source);
	_filters->SetFilter(_entryData->_source, _entryData->_filter);
	_conditions->setCurrentIndex(static_cast<int>(_entryData->_condition));
	_settings->setPlainText(_entryData->_settings);
	_regex->SetRegexConfig(_entryData->_regex);
	const auto filters =
		_entryData->_filter.GetFilters(_entryData->_source);
	_settingSelection->SetSource(filters.empty() ? nullptr : filters.at(0));
	_settingSelection->SetSetting(_entryData->_setting);
	SetWidgetVisibility();

	adjustSize();
	updateGeometry();
}

} // namespace advss
