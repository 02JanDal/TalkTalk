#pragma once

#include <QAbstractItemView>

class ChatView : public QAbstractItemView
{
	Q_OBJECT
public:
	explicit ChatView(QWidget *parent = nullptr);

	void setModel(QAbstractItemModel *model) override;

protected:
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void mouseDoubleClickEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void paintEvent(QPaintEvent *e) override;

public:
	void keyboardSearch(const QString &search) override;
	QRect visualRect(const QModelIndex &index) const override;
	void scrollTo(const QModelIndex &index, ScrollHint hint) override;
	QModelIndex indexAt(const QPoint &point) const override;

public slots:
	void selectAll() override;

protected slots:
	void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) override;
	void rowsInserted(const QModelIndex &parent, int start, int end) override;
	void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) override;
	void horizontalScrollbarAction(int action) override;
	void verticalScrollbarValueChanged(int value) override;
	void columnsRemoved(const QModelIndex &parent, int start, int end);
	void columnsInserted(const QModelIndex &parent, int start, int end);

protected:
	QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
	int horizontalOffset() const override;
	int verticalOffset() const override;
	bool isIndexHidden(const QModelIndex &index) const override;
	void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override;
	QRegion visualRegionForSelection(const QItemSelection &selection) const override;
	int sizeHintForRow(int row) const override;
	int sizeHintForColumn(int column) const override;
	void updateGeometries() override;

private:
	QVector<int> m_columnSizes;
	void updateColumnSizes();
	void updateTotalHeight();

	QRect mapToViewport(const QRect &rect) const;
};
