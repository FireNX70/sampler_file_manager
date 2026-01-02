#include "filename_validator.hpp"

QValidator::State filename_validator_t::validate(QString &input, int &pos) const
{
	if(input.isEmpty()) return QValidator::Intermediate;
	else if(input.contains('/')) return QValidator::Invalid;

	return QValidator::Acceptable;
}
