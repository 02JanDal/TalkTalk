#include "ChatView.h"

#include <QScrollBar>
#include <QPainter>
#include <QAbstractItemModel>
#include <QDebug>
#include <kwordwrap.h>

ChatView::ChatView(QWidget *parent)
	: QAbstractItemView(parent)
{
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

void ChatView::setModel(QAbstractItemModel *m)
{
	if (model())
	{
		disconnect(model(), 0, this, 0);
	}
	QAbstractItemView::setModel(m);
	updateColumnSizes();

	if (model())
	{
		connect(model(), &QAbstractItemModel::columnsInserted, this, &ChatView::columnsInserted);
		connect(model(), &QAbstractItemModel::columnsRemoved, this, &ChatView::columnsRemoved);
	}
}

void ChatView::mousePressEvent(QMouseEvent *e)
{
	QAbstractItemView::mousePressEvent(e);
}
void ChatView::mouseReleaseEvent(QMouseEvent *e)
{
	QAbstractItemView::mouseReleaseEvent(e);
}
void ChatView::mouseDoubleClickEvent(QMouseEvent *e)
{
	QAbstractItemView::mouseDoubleClickEvent(e);
}
void ChatView::mouseMoveEvent(QMouseEvent *e)
{
	QAbstractItemView::mouseMoveEvent(e);
}

void ChatView::paintEvent(QPaintEvent *e)
{
	QPainter painter(viewport());

	if (!model())
	{
		return;
	}

	const QRect visualArea = QRect(horizontalOffset(), verticalOffset(), viewport()->width(), height());

	int y = 0;
	const int colCount = model()->columnCount();
	QFontMetrics metrics = fontMetrics();
	for (int row = 0; row < model()->rowCount(); ++row)
	{
		const int rowHeight = sizeHintForRow(row);
		if (visualArea.top() <= (y + rowHeight) && visualArea.bottom() >= y)
		{
			int x = 0;
			for (int col = 0; col < colCount; ++col)
			{
				const QModelIndex index = model()->index(row, col);
				KWordWrap ww = KWordWrap::formatText(metrics, QRect(0, 0, m_columnSizes.at(col), -1), 0, index.data().toString());
				const QVariant foreground = index.data(Qt::ForegroundRole);
				if (foreground.isValid())
				{
					painter.setBrush(QBrush(foreground.value<QColor>()));
				}
				else
				{
					painter.setBrush(QBrush(Qt::black));
				}
				ww.drawText(&painter, x, y - verticalOffset(), index.data(Qt::TextAlignmentRole).toInt());

				x += m_columnSizes.at(col);
			}
		}
		y += rowHeight;
	}
}

void ChatView::keyboardSearch(const QString &search)
{
}
QRect ChatView::visualRect(const QModelIndex &index) const
{
	if (!index.isValid())
	{
		return QRect();
	}

	QRect rect;
	rect.setWidth(sizeHintForColumn(index.column()));
	rect.setHeight(sizeHintForRow(index.row()));

	int x = 0;
	for (int i = 0; i < index.column(); ++i)
	{
		x += m_columnSizes.at(i);
	}
	rect.setX(x);

	int y = 0;
	if (index.row() < model()->rowCount(index.parent()))
	{
		y += rect.height();
	}
	else
	{
		y += visualRect(index.sibling(index.row() + 1, index.column())).y();
	}
	rect.setY(y - verticalOffset());

	return rect;
}
void ChatView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
}
QModelIndex ChatView::indexAt(const QPoint &point) const
{
	int column;
	int x = 0;
	for (int i = 0; i < m_columnSizes.size(); ++i)
	{
		x += m_columnSizes.at(i);
		if (point.x() < x)
		{
			column = i;
			break;
		}
	}

	for (int row = 0; row < model()->rowCount(); ++row)
	{
		const QModelIndex index = model()->index(row, column);
		const QRect rect = visualRect(index);
		if (rect.contains(point.x(), point.y() + verticalOffset()))
		{
			return index;
		}
		if (rect.y() < 0)
		{
			return QModelIndex();
		}
	}
	return QModelIndex();
}

void ChatView::selectAll()
{
}
void ChatView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
	viewport()->update();
}
void ChatView::rowsInserted(const QModelIndex &parent, int start, int end)
{
	updateTotalHeight();
	viewport()->update();
}
void ChatView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
	Q_ASSERT_X(false, "ChatView::rowsAboutToBeRemoved", "Removal of rows is not supported");
}
void ChatView::horizontalScrollbarAction(int action)
{
}
void ChatView::verticalScrollbarValueChanged(int value)
{
}
QModelIndex ChatView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
	return currentIndex();
}
void ChatView::columnsRemoved(const QModelIndex &parent, int start, int end)
{
	updateColumnSizes();
}
void ChatView::columnsInserted(const QModelIndex &parent, int start, int end)
{
	updateColumnSizes();
}

int ChatView::horizontalOffset() const
{
	return 0;
}
int ChatView::verticalOffset() const
{
	return verticalScrollBar()->value();
}

bool ChatView::isIndexHidden(const QModelIndex &index) const
{
	return false;
}
void ChatView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
}
QRegion ChatView::visualRegionForSelection(const QItemSelection &selection) const
{
	return QRegion();
}

int ChatView::sizeHintForRow(int row) const
{
	QFontMetrics metrics = fontMetrics();
	const QRect formatted = KWordWrap::formatText(metrics, QRect(0, 0, sizeHintForColumn(model()->columnCount() - 1), -1), 0, model()->index(row, m_columnSizes.size()).data(Qt::DisplayRole).toString()).boundingRect();
	return formatted.height();
}
int ChatView::sizeHintForColumn(int column) const
{
	return m_columnSizes.at(column);
}

void ChatView::updateGeometries()
{
	QAbstractItemView::updateGeometries();
	updateColumnSizes();
	updateTotalHeight();
}

void ChatView::updateColumnSizes()
{
	if (!model())
	{
		return;
	}

	m_columnSizes.resize(model()->columnCount());

	if (m_columnSizes.size() == 1)
	{
		m_columnSizes[0] = viewport()->width();
	}
	else if (m_columnSizes.size() > 1)
	{
		// first few columns share 1/5 of the space
		int size = viewport()->width();
		for (int i = 0; i < (m_columnSizes.size() - 1); ++i)
		{
			const int col = std::floor(qreal(viewport()->width()) / qreal(5));
			m_columnSizes[i] = col;
			size -= col;
		}
		// last column gets the last 4/5th of the space
		m_columnSizes[m_columnSizes.size() - 1] = size;
	}
	viewport()->update();
}

void ChatView::updateTotalHeight()
{
	if (!model())
	{
		return;
	}

	bool wasAtBottom = verticalScrollBar()->value() == verticalScrollBar()->maximum();

	int totalHeight = 0;
	for (int i = 0; i < model()->rowCount(); ++i)
	{
		totalHeight += sizeHintForRow(i);
	}
	if (totalHeight < viewport()->height())
	{
		verticalScrollBar()->setRange(0, 0);
	}
	else
	{
		verticalScrollBar()->setRange(0, totalHeight - viewport()->height());
	}
	verticalScrollBar()->setSingleStep(fontMetrics().height());

	if (wasAtBottom)
	{
		verticalScrollBar()->setValue(verticalScrollBar()->maximum());
	}
}
