/***************************************************************************
 *   Copyright (C) 2006-2008, 2014, 2016 by Hanna Knutsson                 *
 *   hanna_k@fmgirl.com                                                    *
 *                                                                         *
 *   This file is part of Eqonomize!.                                      *
 *                                                                         *
 *   Eqonomize! is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Eqonomize! is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with Eqonomize!. If not, see <http://www.gnu.org/licenses/>.    *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "eqonomizevalueedit.h"

#include "budget.h"

#include <cmath>
#include <cfloat>

#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>
#include <QFocusEvent>

#define MAX_VALUE 1000000000000.0

QString calculatedText;
QString last_error;
const EqonomizeValueEdit *calculatedText_object = NULL;

EqonomizeValueEdit::EqonomizeValueEdit(bool allow_negative, QWidget *parent, Budget *budg) : QDoubleSpinBox(parent), budget(budg) {
	init(allow_negative ? -MAX_VALUE : 0.0, MAX_VALUE, 1.0, 0.0, -1, true);
}
EqonomizeValueEdit::EqonomizeValueEdit(double value, bool allow_negative, bool show_currency, QWidget *parent, Budget *budg) : QDoubleSpinBox(parent), budget(budg) {
	init(allow_negative ? -MAX_VALUE : 0.0, MAX_VALUE, 1.0, value, -1, show_currency);
}
EqonomizeValueEdit::EqonomizeValueEdit(double value, int precision, bool allow_negative, bool show_currency, QWidget *parent, Budget *budg) : QDoubleSpinBox(parent), budget(budg) {
	init(allow_negative ? -MAX_VALUE : 0.0, MAX_VALUE, 1.0, value, precision, show_currency);
}
EqonomizeValueEdit::EqonomizeValueEdit(double lower, double step, double value, int precision, bool show_currency, QWidget *parent, Budget *budg) : QDoubleSpinBox(parent), budget(budg) {
	init(lower, MAX_VALUE, step, value, precision, show_currency);
}
EqonomizeValueEdit::EqonomizeValueEdit(double lower, double upper, double step, double value, int precision, bool show_currency, QWidget *parent, Budget *budg) : QDoubleSpinBox(parent), budget(budg) {
	init(lower, upper, step, value, precision, show_currency);
}
EqonomizeValueEdit::~EqonomizeValueEdit() {}

void EqonomizeValueEdit::init(double lower, double upper, double step, double value, int precision, bool show_currency) {
	o_currency = NULL;
	i_precision = precision;
	QDoubleSpinBox::setRange(lower, upper);
	setSingleStep(step);
	setAlignment(Qt::AlignRight);
	if(show_currency) {
		if(budget) {
			if(i_precision < 0) i_precision = budget->defaultCurrency()->fractionalDigits(true);
			setCurrency(budget->defaultCurrency(), true);
		} else {
			if(CURRENCY_IS_PREFIX) {
				s_prefix = QLocale().currencySymbol();
			} else {
				s_suffix = QLocale().currencySymbol();
			}
		}
	}
	if(i_precision < 0) i_precision = MONETARY_DECIMAL_PLACES;
	setDecimals(i_precision);
	setValue(value);
	connect(this, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
	QLineEdit *w = findChild<QLineEdit*>();
	if(w) connect(w, SIGNAL(textChanged(const QString&)), this, SLOT(onTextChanged(const QString&)));
}
void EqonomizeValueEdit::onTextChanged(const QString &text) {
	if(text == s_suffix) {
		QLineEdit *w = findChild<QLineEdit*>();
		if(w) {
			w->setCursorPosition(0);
		}
	}
}
void EqonomizeValueEdit::onEditingFinished() {
	if(hasFocus()) emit returnPressed();
}
void EqonomizeValueEdit::setRange(double lower, double step, int precision) {
	setRange(lower, MAX_VALUE, step, precision);
}
void EqonomizeValueEdit::setRange(double lower, double upper, double step, int precision) {
	i_precision = precision;
	setDecimals(precision);
	QDoubleSpinBox::setRange(lower, upper);
	setSingleStep(step);
}
void EqonomizeValueEdit::setPrecision(int precision) {
	if(precision == i_precision) return;
	i_precision = precision;
	setDecimals(precision);
}
void EqonomizeValueEdit::selectNumber() {
	if(s_prefix.isEmpty() && s_suffix.isEmpty()) {
		selectAll();
		return;
	}
	QLineEdit *w = findChild<QLineEdit*>();
	if(w) {
		QString text = w->text();
		int start = 0;
		int end = text.length();
		if(!s_prefix.isEmpty() && text.startsWith(s_prefix)) {
			start = s_prefix.length();
		}
		if(!s_suffix.isEmpty() && text.endsWith(QString(" ") + s_suffix)) {
			end -= (s_suffix.length() + 1);
		}		
		w->setSelection(end, start - end);
	}
}
void EqonomizeValueEdit::focusInEvent(QFocusEvent *e) {
	if(e->reason() == Qt::TabFocusReason || e->reason() == Qt::BacktabFocusReason) {
		QDoubleSpinBox::focusInEvent(e);
		selectNumber();
	} else {
		QDoubleSpinBox::focusInEvent(e);
	}
	
}
void EqonomizeValueEdit::enterFocus() {
	setFocus(Qt::TabFocusReason);
}
void EqonomizeValueEdit::setCurrency(Currency *currency, bool keep_precision, int as_default, bool is_temporary) {

	if(is_temporary) o_currency = NULL;
	else o_currency = currency;
	
	if(!currency) {
		s_suffix = QString();
		s_prefix = QString();
		setValue(value());
		return;
	}
	
	if((as_default == 0 || (as_default < 0 && currency != budget->defaultCurrency())) || currency->symbol().isEmpty()) {
		s_suffix = currency->code();
		s_prefix = QString();
	} else {
		if(currency && currency->symbolPrecedes()) s_prefix = currency->symbol();
		else s_prefix = QString();
		if(currency && !currency->symbolPrecedes()) s_suffix = currency->symbol();
		else s_suffix = QString();
	}
	if(!keep_precision) {
		setDecimals(currency->fractionalDigits(true));
	} else {
		setValue(value());
	}

}
Currency *EqonomizeValueEdit::currency() {return o_currency;}

QValidator::State EqonomizeValueEdit::validate(QString &input, int &pos) const {
	QString input2 = input;
	int pos2 = pos;
	QValidator::State s = QDoubleSpinBox::validate(input2, pos2);
	if(s == QValidator::Invalid && (pos == 0 || (input[pos - 1] != '[' && input[pos - 1] != ']' && input[pos - 1] != '(' && input[pos - 1] != ')'))) {
		QString str = input2.trimmed();
		if(!s_suffix.isEmpty() && str.endsWith(s_suffix)) {
			str = str.left(str.length() - s_suffix.length());
		}
		if(!s_prefix.isEmpty() && str.endsWith(s_prefix)) {
			str = str.right(str.length() - s_prefix.length());
		}
		str = str.simplified();
		str.remove(' ');
		if(str != input2) {
			if(pos2 > str.length()) pos2 = str.length();
			if(QDoubleSpinBox::validate(str, pos2) == QValidator::Acceptable) return QValidator::Acceptable;
		}
		return QValidator::Intermediate;
	}
	return s;
}

QString EqonomizeValueEdit::textFromValue(double value) const {
	if(!s_suffix.isEmpty()) return s_prefix + QLocale().toString(value, 'f', decimals()) + QString(" ") + s_suffix;
	return s_prefix + QLocale().toString(value, 'f', decimals());
}
double EqonomizeValueEdit::valueFromText(const QString &t) const {
	QString str = t.simplified();
	if(!s_suffix.isEmpty()) {
		str.remove(s_suffix);
	}
	if(!s_prefix.isEmpty()) {
		str.remove(s_prefix);
	}
	str.remove(' ');
	return QDoubleSpinBox::valueFromText(str);
}

void EqonomizeValueEdit::fixup(QString &input) const {
	if(!s_prefix.isEmpty() && input.startsWith(s_prefix)) {
		input = input.right(input.length() - s_prefix.length());
		if(input.isEmpty()) input = QLocale().toString(1);
	}
	if(!s_suffix.isEmpty() && input.endsWith(s_suffix)) {
		input = input.left(input.length() - s_suffix.length());
		if(input.isEmpty()) input = QLocale().toString(1);
	}
	QString calculatedText_pre = input.trimmed();
	input.remove(QRegExp("\\s"));
	input.remove(QLocale().groupSeparator());
	QStringList errors;
	bool calculated = false;
	input = QLocale().toString(fixup_sub(input, errors, calculated), 'f', decimals());
	if(calculated && errors.isEmpty()) {
		calculatedText = calculatedText_pre;
		calculatedText_object = this;
	} else if(calculatedText_object == this) {
		calculatedText_object = NULL;
		calculatedText.clear();
	}
	if(!errors.isEmpty()) {
		QString error;
		for(int i = 0; i < errors.size(); i++) {
			if(i != 0) error += '\n';
			error += errors[i];
		}
		if(last_error == error) {
			last_error = "";
		} else {
			last_error = error;
			QMessageBox::critical((QWidget*) parent(), tr("Error"), error);
		}
	}
}
double EqonomizeValueEdit::fixup_sub(QString &input, QStringList &errors, bool &calculated) const {
	input = input.trimmed();
	if(input.isEmpty()) {
		return 0.0;
	}
	input.replace(QLocale().negativeSign(), '-');
	input.replace(QLocale().positiveSign(), '+');
	int i = input.indexOf(QRegExp("[-+]"), 1);
	if(i >= 1) {
		QStringList terms = input.split(QRegExp("[-+]"));
		i = 0;
		double v = 0.0;
		QList<bool> signs;
		signs << true;
		for(int terms_i = 0; terms_i < terms.size() - 1; terms_i++) {
			i += terms[terms_i].length();
			if(input[i] == '-') signs << false;
			else signs << true;
			i++;
		}
		for(int terms_i = 0; terms_i < terms.size() - 1; terms_i++) {
			if(terms[terms_i].endsWith('*') || terms[terms_i].endsWith('/') || terms[terms_i].endsWith('^')) {
				if(!signs[terms_i + 1]) terms[terms_i] += '-';
				else terms[terms_i] += '+';
				terms[terms_i] += terms[terms_i + 1];
				signs.removeAt(terms_i + 1);
				terms.removeAt(terms_i + 1);
				terms_i--;
			}
		}
		if(terms.size() > 1) {
			for(int terms_i = 0; terms_i < terms.size(); terms_i++) {
				if(terms[terms_i].isEmpty()) {
					if(!signs[terms_i] && terms_i + 1 < terms.size()) {
						signs[terms_i + 1] = !signs[terms_i + 1];
					}
				} else {					
					if(!signs[terms_i]) v -= fixup_sub(terms[terms_i], errors, calculated);
					else v += fixup_sub(terms[terms_i], errors, calculated);
				}
			}			
			calculated = true;
			return v;
		}
	}
	if(input.indexOf("**") >= 0) input.replace("**", "^");
	i = input.indexOf(QRegExp("[*/]"), 0);
	if(i >= 0) {
		QStringList terms = input.split(QRegExp("[*/]"));
		QChar c = '*';
		i = 0;
		double v = 1.0;
		for(int terms_i = 0; terms_i < terms.size(); terms_i++) {
			if(terms[terms_i].isEmpty()) {
				if(c == '/') {
					errors << tr("Empty denominator.");
				} else {
					errors << tr("Empty factor.");
				}
			} else {
				i += terms[terms_i].length();
				if(c == '/') {
					double den = fixup_sub(terms[terms_i], errors, calculated);
					if(den == 0.0) {
						errors << tr("Division by zero.");
					} else {
						v /= den;
					}
				} else {
					v *= fixup_sub(terms[terms_i], errors, calculated);
				}
				if(i < input.length()) c = input[i];
			}
			i++;
		}
		calculated = true;
		return v;
	}
	i = input.indexOf(QLocale().percent());
	if(i >= 0) {
		double v = 0.01;
		if(input.length() > 1) {
			if(i > 0 && i < input.length() - 1) {
				QString str = input.right(input.length() - 1 - i);
				input = input.left(i);
				i = 0;
				v = fixup_sub(str, errors, calculated) * v;
			} else if(i == input.length() - 1) {
				input = input.left(i);
			} else if(i == 0) {
				input = input.right(input.length() - 1);
			}
			v = fixup_sub(input, errors, calculated) * v;
		}
		calculated = true;
		return v;
	}	
	if(budget && o_currency) {
		QString reg_exp_str = "[\\d\\+\\-\\^";
		reg_exp_str += '\\';
		reg_exp_str += QLocale().decimalPoint();
		reg_exp_str += '\\';
		reg_exp_str += QLocale().groupSeparator();
		reg_exp_str += "]";
		int i = input.indexOf(QRegExp(reg_exp_str));
		if(i >= 1) {
			QString scur = input.left(i).trimmed();
			Currency *cur = budget->findCurrency(scur);
			if(!cur && budget->defaultCurrency()->symbol(false) == scur) cur = budget->defaultCurrency();
			if(!cur) cur = budget->findCurrencySymbol(scur, true);
			if(cur) {
				QString value = input.right(input.length() - i);				
				double v = fixup_sub(value, errors, calculated);
				if(cur != o_currency) {
					v = cur->convertTo(v, o_currency);
				}
				calculated = true;
				return v;
			}
			errors << tr("Unknown or ambiguous currency, or unrecognized characters, in expression: %1.").arg(scur);
		}
		if(i >= 0) {
			i = input.lastIndexOf(QRegExp(reg_exp_str));
			if(i >= 0 && i < input.length() - 1) {
				QString scur = input.right(input.length() - (i + 1)).trimmed();
				Currency *cur = budget->findCurrency(scur);
				if(!cur && budget->defaultCurrency()->symbol(false) == scur) cur = budget->defaultCurrency();
				if(!cur) cur = budget->findCurrencySymbol(scur, true);
				if(cur) {
					QString value = input.left(i + 1);
					double v = fixup_sub(value, errors, calculated);
					if(cur != o_currency) {
						v = cur->convertTo(v, o_currency);
					}
					calculated = true;
					return v;
				}
				errors << tr("Unknown or ambiguous currency, or unrecognized characters, in expression: %1.").arg(scur);
			}
		} else {
			Currency *cur = budget->findCurrency(input);
			if(!cur && budget->defaultCurrency()->symbol(false) == input) cur = budget->defaultCurrency();
			if(!cur) cur = budget->findCurrencySymbol(input, true);
			if(cur) {
				double v = 1.0;
				if(cur != o_currency) {
					v = cur->convertTo(v, o_currency);
				}
				calculated = true;
				return v;
			}
			errors << tr("Unknown or ambiguous currency, or unrecognized characters, in expression: %1.").arg(input);
		}
	}
	i = input.indexOf('^', 0);
	if(i >= 0 && i != input.length() - 1) {
		QString base = input.left(i);
		if(base.isEmpty()) {
			errors << tr("Empty base.");
		} else {
			QString exp = input.right(input.length() - (i + 1));
			double v;
			if(exp.isEmpty()) {
				errors << tr("Error"), tr("Empty exponent.");
				v = 1.0;
			} else {
				v = pow(fixup_sub(base, errors, calculated), fixup_sub(exp, errors, calculated));
			}
			calculated = true;
			return v;
		}
	}
	
	if(!budget || !o_currency) {
		QString reg_exp_str = "[^\\d\\+\\-";
		reg_exp_str += '\\';
		reg_exp_str += QLocale().decimalPoint();
		reg_exp_str += '\\';
		reg_exp_str += QLocale().groupSeparator();
		reg_exp_str += "]";
		i = input.indexOf(QRegExp(reg_exp_str));
		if(i >= 0) {
			errors << tr("Unrecognized characters in expression.");
		}
	}
	input.replace('-', QLocale().negativeSign());
	input.replace('+', QLocale().positiveSign());
	return QLocale().toDouble(input);
}

