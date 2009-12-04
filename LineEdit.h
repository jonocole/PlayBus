#ifndef LINE_EDIT_H_
#define LINE_EDIT_H_

#include <QLineEdit>

struct LineEdit : public QLineEdit {
Q_OBJECT
public:
    LineEdit():QLineEdit(){
        connect( this, SIGNAL(returnPressed()), SLOT(onRetPressed()));
    }

private slots:
    void onRetPressed() {
        emit returnPressed( text() );
    }

signals:
    void returnPressed( const QString& text );
};

#endif //LINE_EDIT_H_
