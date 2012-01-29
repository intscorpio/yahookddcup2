#include "binsvd_pred.h"

QHash<int, QVector<float> > user_factors;
QHash<int, QVector<float> > item_factors;
QHash<int, QList<int> > user_negatives;
QHash<int, QList<int> > user_positives;

QHash<int, QList<int> > load_negatives(QString file_name)
{
    QHash<int, QList<int> > user_negatives;

    printf("Start loading set from %s ... ", qPrintable(file_name));

    QFile binfile(file_name + ".bin");
    if (!binfile.exists())
    {
        QFile file(file_name + ".txt");
        file.open(QFile::ReadOnly);
        QTextStream in(&file);

        int u = 0, i = 0;
        QStringList list;
        while (!in.atEnd())
        {
            QString line = in.readLine();
//            printf(qPrintable(line+'\n'));
            if (line.contains('|'))
            {
                list = line.split('|');
                u = list.at(0).toInt();
                QList<int> items;
                user_negatives.insert(u, items);
            }
            else
            {
                list = line.split('\t');
                i = list.at(0).toInt();
                user_negatives[u].append(i);
            }
        }
        file.close();

        // serializationRsHash
        binfile.open(QFile::WriteOnly);
        QDataStream bin(&binfile);
        bin << user_negatives;
        binfile.close();
    }
    else
    {
        // deserialization
        binfile.open(QFile::ReadOnly);
        QDataStream bin(&binfile);
        bin >> user_negatives;
        binfile.close();
    }
    printf("OK\n");
    return user_negatives;
}

QHash<int, QList<int> > load_positives(RsHash train)
{
    QHash<int, QList<int> > positives;

    RsHash::const_iterator it;
    for(it = train.constBegin(); it != train.constEnd(); ++it)
    {
        positives[it.key()] = QList<int>();
        QHash<int, float>::const_iterator it2;
        for(it2 = it.value().constBegin(); it2 != it.value().constEnd(); ++it2)
        {
            if (it2.value() >= 80) {
                positives[it.key()].append(it2.key());
            }
        }
    }

    return positives;
}

void create_factors(QHash<int, QVector<float> > &factors, RsHash train, int fact_n, bool u_flag)
{
    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    // fill all user ids
    QList<int> elements;
    if (u_flag)
    {
        elements = train.keys();
    }
    // fill all item ids
    else
    {
        QHash<int, int> items;
        RsHash::const_iterator it;
        for(it = train.constBegin(); it != train.constEnd(); ++it)
        {
            QHash<int, float>::const_iterator it2;
            for(it2 = it.value().constBegin(); it2 != it.value().constEnd(); ++it2)
            {
                if (!items.contains(it2.key()))
                {
                    items[it2.key()] = 0;
                }
            }
        }
        elements = items.keys();
    }

    // fill factors with small randoms
    QList<int>::const_iterator it;
    for(it = elements.constBegin(); it != elements.constEnd(); ++it)
    {
        factors[*it] = QVector<float>();
        for(int fi = 0; fi < fact_n; fi++)
        {
            float rand_v = (float)(qrand() % 10) / 1000 + 0.0005;
            factors[*it].append(rand_v);
        }
        if (factors[*it].count() < fact_n)
            printf("%d  ", factors[*it].count());
    }
}

float dot_product(QVector<float> u_f, QVector<float> i_f, int fact_n)
{
    float dot_prod = 0;
    for(int i = 0; i < fact_n; i++)
    {
        dot_prod += u_f[i] * i_f[i];
    }
    return dot_prod;
}

void binsvd_pred::study(RsHash train, bool verbose)
{
    if (verbose) printf("Start binsvd studying...\n");

    int steps = 1, fact_n = 10;
    float alfa = 0.01, lambda = 0.01;

    // create and fill data structures
    user_negatives = load_negatives("../../train_negatives_sample");
    user_positives = load_positives(train);
    create_factors(user_factors, train, fact_n, 1);
    create_factors(item_factors, train, fact_n, 0);

    // by step
    for (int st = 1; st <= steps; st++)
    {
        // by user
        QHash<int, QList<int> >::const_iterator it;
        for(it = user_positives.constBegin(); it != user_positives.constEnd(); ++it)
        {
            int u = it.key();
            QList<int> u_pos = it.value();
            QList<int> u_neg = user_negatives[u];

            // by negative and positive items
            for(int r = -1; r <= 1; r += 2)
            {
                QList<int> items_list;
                if (r == -1) items_list = u_neg;
                else items_list = u_pos;

                QList<int>::const_iterator it2;
                for(it2 = items_list.constBegin(); it2 != items_list.constEnd(); ++it2)
                {
                    int i = *it2;

                    if (user_factors[u].count() < fact_n || item_factors[i].count() < fact_n)
                    {
                        printf("%d  %d,  %d  %d\n", u, user_factors[u].count(), i, item_factors[i].count());
                    }

                    float pr = dot_product(user_factors[u], item_factors[i], fact_n);
                    float err = r - pr;

                    // by factor
                    for(int fi = 0; fi < fact_n; fi++)
                    {
                        float user_f = user_factors[u][fi];
                        float item_f = item_factors[i][fi];
                        user_f = user_f + alfa * (err * item_f - lambda * user_f);
                        item_f = item_f + alfa * (err * user_f - lambda * item_f);
                    }
                }
            }

        }
    }
    if (verbose) printf("ok\n");
}

void free_memory(QHash<int, float*> &factors, QHash<int, float*> &factors2)
{
    /*for(int i = 0; i < factors.count(); i++)     {
        factors
    }*/
}


















