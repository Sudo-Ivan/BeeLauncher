#include <QTest>
#include <QStandardItemModel>
#include "ui/shell/ShellInstanceModel.h"
#include "InstanceList.h"

class ShellModel_test : public QObject {
    Q_OBJECT
   private:
    static void add(QStandardItemModel& m, const QString& id, const QString& name, const QString& group)
    {
        auto* it = new QStandardItem(name);
        it->setData(id, InstanceList::InstanceIDRole);
        it->setData(group, InstanceList::GroupRole);
        m.appendRow(it);
    }

   private slots:
    void test_filter()
    {
        QStandardItemModel src;
        add(src, "a", "Alpha", "G1");
        add(src, "b", "Beta", "G2");
        ShellInstanceModel proxy(&src);
        QCOMPARE(proxy.rowCount(), 2);
        proxy.setFilterText("alpha");
        QCOMPARE(proxy.rowCount(), 1);
        proxy.setFilterText("");
        QCOMPARE(proxy.rowCount(), 2);
    }

    void test_group_role()
    {
        QStandardItemModel src;
        add(src, "a", "Alpha", "G1");
        add(src, "b", "Beta", "G2");
        ShellInstanceModel proxy(&src);
        QCOMPARE(proxy.data(proxy.index(0, 0), ShellInstanceModel::GroupRole2).toString(), QString("G1"));
        QCOMPARE(proxy.data(proxy.index(1, 0), ShellInstanceModel::GroupRole2).toString(), QString("G2"));
        QCOMPARE(proxy.data(proxy.index(0, 0), ShellInstanceModel::InstanceIdRole).toString(), QString("a"));
        QCOMPARE(proxy.instanceIdAt(0), QString("a"));
    }

    void test_sort_name()
    {
        QStandardItemModel src;
        add(src, "b", "Beta", "");
        add(src, "a", "Alpha", "");
        ShellInstanceModel proxy(&src);
        QCOMPARE(proxy.data(proxy.index(0, 0), ShellInstanceModel::NameRole).toString(), QString("Alpha"));
    }
};

QTEST_GUILESS_MAIN(ShellModel_test)
#include "ShellModel_test.moc"
