#pragma once

#include <stdexcept>
#include <QString>

namespace RPGMakerDecrypter {

/// Base class for all decrypter errors.
class DecrypterException : public std::runtime_error
{
public:
    explicit DecrypterException(const QString& message)
        : std::runtime_error(message.toStdString())
    {
    }
};

/// RGSSAD archive is malformed or unsupported.
class InvalidArchiveException final : public DecrypterException
{
public:
    explicit InvalidArchiveException(const QString& message)
        : DecrypterException(message)
    {
    }
};

/// System.json encryption key was missing or malformed.
class EncryptionKeyException final : public DecrypterException
{
public:
    explicit EncryptionKeyException(const QString& message)
        : DecrypterException(message)
    {
    }
};

/// A user-requested operation cannot be performed.
class InvalidUsageException final : public DecrypterException
{
public:
    explicit InvalidUsageException(const QString& message)
        : DecrypterException(message)
    {
    }
};

/// Raised when the user cancels a long-running operation.
class OperationCancelledException final : public DecrypterException
{
public:
    OperationCancelledException()
        : DecrypterException(QStringLiteral("The operation was cancelled."))
    {
    }
};

} // namespace RPGMakerDecrypter
