#pragma once
#include "macro-action-edit.hpp"
#include "variable-text-edit.hpp"
#include "source-selection.hpp"

#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>

class MacroActionFilter : public MacroAction {
public:
	MacroActionFilter(Macro *m) : MacroAction(m) {}
	bool PerformAction();
	void LogAction() const;
	bool Save(obs_data_t *obj) const;
	bool Load(obs_data_t *obj);
	std::string GetShortDesc() const;
	std::string GetId() const { return id; };
	static std::shared_ptr<MacroAction> Create(Macro *m)
	{
		return std::make_shared<MacroActionFilter>(m);
	}

	enum class Action {
		ENABLE,
		DISABLE,
		SETTINGS,
	};

	SourceSelection _source;
	OBSWeakSource _filter;
	Action _action = Action::ENABLE;
	StringVariable _settings = "";

private:
	void ResolveVariables();
	std::string _filterName = "";

	static bool _registered;
	static const std::string id;

	friend class MacroActionFilterEdit;
};

class MacroActionFilterEdit : public QWidget {
	Q_OBJECT

public:
	MacroActionFilterEdit(
		QWidget *parent,
		std::shared_ptr<MacroActionFilter> entryData = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroAction> action)
	{
		return new MacroActionFilterEdit(
			parent,
			std::dynamic_pointer_cast<MacroActionFilter>(action));
	}

private slots:
	void SourceChanged(const SourceSelection &);
	void FilterChanged(const QString &);
	void ActionChanged(int value);
	void GetSettingsClicked();
	void SettingsChanged();
signals:
	void HeaderInfoChanged(const QString &);

protected:
	SourceSelectionWidget *_sources;
	QComboBox *_filters;
	QComboBox *_actions;
	QPushButton *_getSettings;
	VariableTextEdit *_settings;
	std::shared_ptr<MacroActionFilter> _entryData;

private:
	void SetWidgetVisibility(bool);
	bool _loading = true;
};
