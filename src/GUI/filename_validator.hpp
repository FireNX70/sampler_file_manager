#ifndef QT_FILENAME_VALIDATOR_HEADER_INCLUDE_GUARD
#define QT_FILENAME_VALIDATOR_HEADER_INCLUDE_GUARD

#include <QValidator>

class filename_validator_t: public QValidator
{
	QValidator::State validate(QString &input, int &pos) const override;
};
#endif
