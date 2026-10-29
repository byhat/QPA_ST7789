/**
 * @file Plugin.hpp
 * @brief Точка входа QPA-плагина qpast7789.
 */

#pragma once

#if __has_include(<QtGui/private/qplatformintegrationplugin_p.h>)
#include <QtGui/private/qplatformintegrationplugin_p.h>
#elif __has_include(<qpa/qplatformintegrationplugin.h>)
#include <qpa/qplatformintegrationplugin.h>
#endif

/**
 * @class QpaSt7789Plugin
 * @brief Фабрика интеграций; ключи заданы в metadata.json ("qpast7789").
 */
class QpaSt7789Plugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QPA.QPlatformIntegrationFactoryInterface.5.3"
                      FILE "metadata.json")
public:
    /**
     * @brief Создаёт интеграцию для запрошенного ключа платформы.
     * @param key ключ платформы (игнорируется)
     * @param parameters параметры из QT_QPA_PLATFORM (игнорируются)
     * @return новая интеграция; владение переходит Qt
     */
    QPlatformIntegration *create(const QString &key, const QStringList &parameters) override;
};
