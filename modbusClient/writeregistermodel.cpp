#include "writeregistermodel.h"

WriteRegisterModel::WriteRegisterModel(int beginAddress, int valueNumber, QObject *parent)
    : QAbstractTableModel(parent)
    , m_beginAddress(beginAddress)
    , m_valueNumber(valueNumber)
    , m_coils(beginAddress + valueNumber, false)
    , m_holdingRegisters(beginAddress + valueNumber, 0)
    , m_discreteRegisters(beginAddress + valueNumber, 0)
    , m_InputRegisters(beginAddress + valueNumber, 0)
{

}

int WriteRegisterModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_valueNumber;
}

int WriteRegisterModel::columnCount(const QModelIndex &/*parent*/) const
{
    return ColumnCount;
}

QVariant WriteRegisterModel::data(const QModelIndex &index, int role) const
{

    if (!index.isValid() || index.row() >= m_valueNumber || index.column() >= ColumnCount)
        return QVariant();
    if (m_InputRegisters.size() < index.row() + m_beginAddress + 2 || m_holdingRegisters.size() < index.row() + m_beginAddress + 2)
        return QVariant();

    if (index.column() == NumColumn && role == Qt::DisplayRole)
        return QString("%1").arg(QString::number(index.row() + m_beginAddress));

    else if (index.column() == CoilsColumn && role == Qt::CheckStateRole)
        return m_coils.at(index.row() + m_beginAddress) ? Qt::Checked : Qt::Unchecked;
    else if (index.column() == DiscreteColum && role == Qt::DisplayRole)
        return QString("%1").arg(QString::number(m_discreteRegisters.at(index.row() + m_beginAddress)));

    else if (index.column() == InputColumn && role == Qt::DisplayRole){
        if (valueType == Float){
            if (index.row() % 2 == 0){
                uint uintValueTmp = (m_InputRegisters.at(index.row() + m_beginAddress + 1) << 16) + m_InputRegisters.at(index.row() + m_beginAddress);
                float floatValueTmp = *(float *)&uintValueTmp;
                return QString("%1").arg(QString::number(floatValueTmp));
            }
            else
                return QVariant();
        }
        else{
            quint16 valueTmp = m_InputRegisters.at(index.row() + m_beginAddress);
            if (valueType == Binary)
                return QString("0b%1").arg(QString::number(valueTmp, 2));
            if (valueType == Hex)
                return QString("0x%1").arg(QString::number(valueTmp, 16));
            else if (valueType == Unsigned)
                return QString("%1").arg(QString::number(valueTmp, 10));
            else{
                qint16 valueTmpT = *(qint16 *)&valueTmp;
                return QString("%1").arg(QString::number(valueTmpT, 10));
            }
        }
    }

    else if (index.column() == HoldingColumn && role == Qt::DisplayRole){
        if (valueType == Float){
            if (index.row() % 2 == 0){
                uint uintValueTmp = (m_holdingRegisters.at(index.row() + m_beginAddress + 1) << 16) + m_holdingRegisters.at(index.row() + m_beginAddress);
                float floatValueTmp = *(float *)&uintValueTmp;
                return QString("%1").arg(QString::number(floatValueTmp));
            }
            else
                return QVariant();
        }
        else{
            quint16 valueTmp = m_holdingRegisters.at(index.row() + m_beginAddress);
            if (valueType == Binary)
                return QString("0b%1").arg(QString::number(valueTmp, 2));
            if (valueType == Hex)
                return QString("0x%1").arg(QString::number(valueTmp, 16));
            else if (valueType == Unsigned)
                return QString("%1").arg(QString::number(valueTmp, 10));
            else{
                qint16 valueTmpT = *(qint16 *)&valueTmp;
                return QString("%1").arg(QString::number(valueTmpT, 10));
            }
        }
    }
    return QVariant();
}

QVariant WriteRegisterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Horizontal) {
        switch (section) {
        case NumColumn:return QStringLiteral("地址");
        case CoilsColumn:return QStringLiteral("Coils");
        case DiscreteColum:return QStringLiteral("Discrete Inputs");
        case InputColumn:return QStringLiteral("Input Registers");
        case HoldingColumn:return QStringLiteral("Holding Registers");
        default:break;
        }
    }
    return QVariant();
}

bool WriteRegisterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_valueNumber || index.column() >= ColumnCount)
        return false;

    bool result = false;
    if (index.column() == CoilsColumn && role == Qt::CheckStateRole) { // coils
        value.toUInt() != 0 ? m_coils.setBit(index.row() + m_beginAddress) : m_coils.clearBit(index.row() + m_beginAddress);
        emit dataChanged(index, index);
        return true;
    }
    if (index.column() == DiscreteColum && role == Qt::DisplayRole) {
        quint16 newValue = value.toString().toUShort(&result);
        if (result) m_discreteRegisters[index.row() + m_beginAddress] = newValue;

        emit dataChanged(index, index);
        return result;
    }
    if (index.column() == InputColumn && role == Qt::DisplayRole) {

        quint16 newValue = value.toString().toUShort(&result);
        if (result) m_InputRegisters[index.row() + m_beginAddress] = newValue;

        emit dataChanged(index, index);
        return result;
    }
    if (index.column() == HoldingColumn && role == Qt::EditRole) { // holding registers
        if (salveChangeData){
            quint16 newValue = value.toString().toUShort(&result);
            if (result) m_holdingRegisters[index.row() + m_beginAddress] = newValue;

            emit dataChanged(index, index);
            return result;
        }
        else{
            int intNewValue = value.toInt(&result);
            if (result){
                m_holdingRegisters[index.row() + m_beginAddress] = (quint16)intNewValue;
                m_holdingRegisters[index.row() + m_beginAddress + 1] = (quint16)(intNewValue >> 16);
            }
            else{
                float floatNewValue = value.toFloat(&result);
                if (result){
                    int uintNewValue = *(int *)&floatNewValue;
                    m_holdingRegisters[index.row() + m_beginAddress] = (quint16)uintNewValue;
                    m_holdingRegisters[index.row() + m_beginAddress + 1] = (quint16)(uintNewValue >> 16);
                }
            }
            emit dataChanged(index, index);
            return result;
        }
    }
    return false;
}

Qt::ItemFlags WriteRegisterModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_valueNumber || index.column() >= ColumnCount)
        return QAbstractTableModel::flags(index);

    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    if (index.column() == NumColumn)
        return flags | Qt::ItemIsEnabled;
    if (index.column() == CoilsColumn)
        return flags | Qt::ItemIsUserCheckable;
    if (index.column() == DiscreteColum)
        return flags | Qt::ItemIsEnabled;
    if (index.column() == InputColumn)
        return flags | Qt::ItemIsEnabled;
    if (index.column() == HoldingColumn)
        return flags | Qt::ItemIsEditable;

    return flags;
}

void WriteRegisterModel::setStartAddress(int address)
{
    m_beginAddress = address;

    m_coils.resize(m_valueNumber + m_beginAddress);
    m_discreteRegisters.resize(m_valueNumber + m_beginAddress);
    m_InputRegisters.resize(m_valueNumber + m_beginAddress);
    m_holdingRegisters.resize(m_valueNumber + m_beginAddress);
    emit updateViewport();
}

void WriteRegisterModel::setNumberOfValues(int ValueNum)
{
    m_valueNumber = ValueNum;

    m_coils.resize(m_valueNumber + m_beginAddress);
    m_discreteRegisters.resize(m_valueNumber + m_beginAddress);
    m_InputRegisters.resize(m_valueNumber + m_beginAddress);
    m_holdingRegisters.resize(m_valueNumber + m_beginAddress);
    emit updateViewport();
}
