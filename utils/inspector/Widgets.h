#ifndef WIDGETS_H_
#define WIDGETS_H_

#include <QLineEdit>
struct LineEdit : public QLineEdit {
Q_OBJECT
public:
    LineEdit():QLineEdit(){
        connect( this, SIGNAL(returnPressed()), SLOT(onRetPressed()));
    }

private slots:
    void onRetPressed() {
        emit returnPressed( text().toUtf8() );
    }

signals:
    void returnPressed( const QByteArray& text );
};

#include <QTextEdit>
struct TextEdit : public QTextEdit {
Q_OBJECT
public:
    TextEdit():QTextEdit(){}

public slots:
    void append( const QByteArray& ba )
    {
        QTextEdit::append( QString( ba ));
    }
};

#endif //WIDGETS_H_
