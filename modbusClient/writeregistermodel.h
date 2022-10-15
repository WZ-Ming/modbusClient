#pragma once

#include <QAbstractItemModel>
#include <QBitArray>
#include <QObject>
#include <QDebug>

class WriteRegisterModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	WriteRegisterModel(int m_beginAddress, int m_valueNumber, QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	QBitArray m_discreteRegisters, m_coils;
	QVector<quint16> m_InputRegisters, m_holdingRegisters;

	enum VALUETYPE{ Signed = 0, Float = 1, Binary = 2, Unsigned = 10, Hex = 16 }valueType;
	enum { NumColumn = 0, CoilsColumn = 1, DiscreteColum = 2, InputColumn = 3, HoldingColumn = 4 };

	bool salveChangeData = false;

	public slots:
	void setStartAddress(int address);
	void setNumberOfValues(int ValueNum);

signals:
	void updateViewport();

private:
	const int ColumnCount = 5;
	int m_beginAddress, m_valueNumber;
};
