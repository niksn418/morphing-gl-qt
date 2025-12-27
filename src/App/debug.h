#include <QDebug>

#define check(cmd)							        \
{ 											        \
	bool status = cmd; 						        \
	if (!status) { 							        \
		qDebug() << "ERROR: " << (#cmd) << '\n';    \
	} 										        \
}
