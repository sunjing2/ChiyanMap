#include "state/LanguageManager.h"
#include "state/MapRenderState.h"
#include "state/SeedMapManager.h"
#include "mod/ChiyanMap.h"
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <nlohmann/json.hpp>
#include <ll/api/i18n/I18n.h>
#include <mutex>

using json = nlohmann::json;

namespace LanguageManager {
    std::string g_currentLanguage = "en_US";
    std::vector<std::pair<std::string, std::string>> g_availableLanguages;
    static std::unordered_map<std::string, std::string> g_translationCache;
    static std::mutex g_cacheMutex;
    static std::filesystem::path g_langDir;

    // 内置多语言字典兜底 (覆盖 16 种语言，彻底杜绝界面出现未翻译 raw key)
    static const std::unordered_map<std::string, std::unordered_map<std::string, std::string>> g_builtinTranslations = {
        {"HOTKEY_CANNOT_CLEAR", {
            {"zh_CN", "该快捷键不可清除"},
            {"zh_TW", "該快捷鍵不可清除"},
            {"en_US", "This hotkey cannot be cleared"},
            {"de",    "Dieses Tastenkürzel kann nicht gelöscht werden"},
            {"es",    "Este atajo no se puede borrar"},
            {"fr",    "Ce raccourci ne peut pas être effacé"},
            {"id",    "Tombol pintas ini tidak dapat dihapus"},
            {"it",    "Questa scorciatoia non può essere cancellata"},
            {"ja",    "このショートカットキーは消去できません"},
            {"ko",    "이 단축키는 지ул 수 없습니다"},
            {"pt_BR", "Este atalho não pode ser limpo"},
            {"ru",    "Эту горячую клавишу нельзя очистить"},
            {"th",    "ไม่สามารถลบปุ่มลัดนี้ได้"},
            {"tr",    "Bu kısayol tuşu temizlenemez"},
            {"uk",    "Цю гарячу клавішу не можна очистити"},
            {"vi",    "Không thể xóa phím tắt này"}
        }},
        {"EXPORT_MAP_PNG", {
            {"zh_CN", "导出地图为 PNG"},
            {"zh_TW", "匯出地圖為 PNG"},
            {"en_US", "Export Map as PNG"},
            {"de",    "Karte als PNG exportieren"},
            {"es",    "Exportar mapa como PNG"},
            {"fr",    "Exporter la carte en PNG"},
            {"id",    "Ekspor Peta sebagai PNG"},
            {"it",    "Esporta mappa come PNG"},
            {"ja",    "マップをPNGとしてエクスポート"},
            {"ko",    "지도를 PNG로 내보내기"},
            {"pt_BR", "Exportar Mapa como PNG"},
            {"ru",    "Экспортировать карту как PNG"},
            {"th",    "ส่งออกแผนที่เป็น PNG"},
            {"tr",    "Haritayı PNG Olarak Dışa Aktar"},
            {"uk",    "Експортувати мапу як PNG"},
            {"vi",    "Xuất bản đồ sang PNG"}
        }},
        {"EXPORT_SCREEN_TITLE", {
            {"zh_CN", "世界地图 PNG 导出"},
            {"zh_TW", "世界地圖 PNG 匯出"},
            {"en_US", "World Map PNG Export"},
            {"de",    "Weltkarte PNG-Export"},
            {"es",    "Exportación PNG del mapa mundial"},
            {"fr",    "Exportation PNG de la carte du monde"},
            {"id",    "Ekspor PNG Peta Dunia"},
            {"it",    "Esportazione PNG mappa del mondo"},
            {"ja",    "ワールドマップ PNG エクスポート"},
            {"ko",    "월드맵 PNG 내보내기"},
            {"pt_BR", "Exportação PNG do Mapa-Múndi"},
            {"ru",    "Экспорт карты мира в PNG"},
            {"th",    "ส่งออกแผนที่โลกเป็น PNG"},
            {"tr",    "Dünya Haritası PNG Dışa Aktarma"},
            {"uk",    "Експорт мапи світу в PNG"},
            {"vi",    "Xuất bản đồ thế giới sang PNG"}
        }},
        {"EXPORT_OPT_FULL", {
            {"zh_CN", "强制全图导出"},
            {"zh_TW", "強制全圖匯出"},
            {"en_US", "Force Full Map"},
            {"de",    "Gesamte Karte erzwingen"},
            {"es",    "Forzar mapa completo"},
            {"fr",    "Forcer la carte complète"},
            {"id",    "Paksa Peta Penuh"},
            {"it",    "Forza mappa intera"},
            {"ja",    "マップ全体を強制出力"},
            {"ko",    "전체 지도 강제 내보내기"},
            {"pt_BR", "Forçar Mapa Completo"},
            {"ru",    "Вся карта целиком"},
            {"th",    "บังคับส่งออกแผนที่ทั้งหมด"},
            {"tr",    "Tam Haritayı Zorla"},
            {"uk",    "Примусово вся мапа"},
            {"vi",    "Buộc xuất toàn bộ bản đồ"}
        }},
        {"EXPORT_OPT_FULL_DESC", {
            {"zh_CN", "即使已选择了地图区域，也强制导出整张已探索的地图。"},
            {"zh_TW", "即使已選擇了地圖區域，也強制匯出整張已探索的地圖。"},
            {"en_US", "Export the entire map even if a map selection has been made."},
            {"de",    "Exportiert die gesamte Karte, auch wenn eine Auswahl getroffen wurde."},
            {"es",    "Exporta el mapa completo incluso si se ha realizado una selección en el mapa."},
            {"fr",    "Exporte la carte entière même si une sélection a été effectuée."},
            {"id",    "Ekspor seluruh peta meskipun area peta telah dipilih."},
            {"it",    "Esporta l'intera mappa anche se è stata effettuata una selezione."},
            {"ja",    "範囲選択が行われている場合でも、探索済みのマップ全体をエクスポートします。"},
            {"ko",    "지도 선택 영역이 있더라도 전체 지도를 내보냅니다."},
            {"pt_BR", "Exporta o mapa inteiro mesmo que uma seleção tenha sido feita."},
            {"ru",    "Экспортировать всю карту, даже если была выбрана область карты."},
            {"th",    "ส่งออกแผนที่ทั้งหมดแม้ว่าจะมีการเลือกพื้นที่บนแผนที่ไว้ก็ตาม"},
            {"tr",    "Harita seçimi yapılmış olsa bile tüm haritayı dışa aktarır."},
            {"uk",    "Експортувати всю мапу, навіть якщо вибрано певну область."},
            {"vi",    "Xuất toàn bộ bản đồ ngay cả khi đã chọn một vùng bản đồ."}
        }},
        {"EXPORT_OPT_MULTI", {
            {"zh_CN", "多张无缩放图像"},
            {"zh_TW", "多張無縮放圖像"},
            {"en_US", "Multiple Unscaled Images"},
            {"de",    "Mehrere unskalierte Bilder"},
            {"es",    "Múltiples imágenes sin escalar"},
            {"fr",    "Plusieurs images non mises à l'échelle"},
            {"id",    "Banyak Gambar Tanpa Skala"},
            {"it",    "Immagini multiple non ridimensionate"},
            {"ja",    "複数の非縮小画像"},
            {"ko",    "여러 개의 원본 크기 이미지"},
            {"pt_BR", "Múltiplas Imagens Sem Escala"},
            {"ru",    "Несколько изображений без сжатия"},
            {"th",    "หลายรูปภาพโดยไม่ย่อขนาด"},
            {"tr",    "Çoklu Ölçeklenmemiş Görüntüler"},
            {"uk",    "Кілька немасштабованих зображень"},
            {"vi",    "Nhiều hình ảnh nguyên gốc"}
        }},
        {"EXPORT_OPT_MULTI_DESC", {
            {"zh_CN", "无论地图多大，均按原始分辨率拆分为多张无缩放图像导出，不受单张图像的内存限制。"},
            {"zh_TW", "無論地圖多大，均按原始解析度拆分為多張無縮放圖像匯出，不受單張圖像的記憶體限制。"},
            {"en_US", "Export the map as multiple unscaled images no matter how big it is. Doesn't have the memory limitations of a single image."},
            {"de",    "Exportiert die Karte unabhängig von ihrer Größe als mehrere unskalierte Bilder ohne Speicherbegrenzung eines Einzelbildes."},
            {"es",    "Exporta el mapa como múltiples imágenes sin escalar sin importar su tamaño. No tiene las limitaciones de memoria de una sola imagen."},
            {"fr",    "Exporte la carte en plusieurs images non réduites quelle que soit sa taille, sans les limites de mémoire d'une seule image."},
            {"id",    "Ekspor peta sebagai beberapa gambar tanpa skala berapa pun ukurannya. Tidak memiliki batasan memori seperti gambar tunggal."},
            {"it",    "Esporta la mappa in più immagini non ridimensionate indipendentemente dalle dimensioni, senza i limiti di memoria di una singola immagine."},
            {"ja",    "マップの大きさに関わらず、複数の非縮小画像としてエクスポートします。単一画像のメモリ制限を受けません。"},
            {"ko",    "지도 크기에 상관없이 여러 개의 원본 크기 이미지로 내보냅니다. 단일 이미지의 메모리 제한을 받지 않습니다."},
            {"pt_BR", "Exporta o mapa como várias imagens sem escala, independentemente do tamanho. Não possui as limitações de memória de uma única imagem."},
            {"ru",    "Экспортирует карту как несколько изображений без масштабирования независимо от размера, без ограничений памяти одного изображения."},
            {"th",    "ส่งออกแผนที่เป็นหลายรูปภาพโดยไม่ย่อขนาดไม่ว่าจะใหญ่แค่ไหน ไม่มีข้อจำกัดด้านหน่วยความจำเหมือนรูปภาพเดี่ยว"},
            {"tr",    "Haritayı ne kadar büyük olursa olsun birden fazla ölçeklenmemiş görüntü olarak dışa aktarır. Tek görüntünün bellek sınırlarına takılmaz."},
            {"uk",    "Експортує мапу як кілька немасштабованих зображень незалежно від її розміру, без обмежень пам'яті одного зображення."},
            {"vi",    "Xuất bản đồ thành nhiều ảnh nguyên gốc bất kể kích thước lớn đến đâu, không bị giới hạn bộ nhớ như một ảnh đơn."}
        }},
        {"EXPORT_OPT_OPEN_FOLDER", {
            {"zh_CN", "导出后自动打开文件夹"},
            {"zh_TW", "匯出後自動打開資料夾"},
            {"en_US", "Open Folder After Export"},
            {"de",    "Ordner nach dem Export öffnen"},
            {"es",    "Abrir carpeta tras exportar"},
            {"fr",    "Ouvrir le dossier après l'export"},
            {"id",    "Buka Folder Setelah Ekspor"},
            {"it",    "Apri cartella dopo l'esportazione"},
            {"ja",    "エクスポート後にフォルダを開く"},
            {"ko",    "내보내기 후 폴더 자동 열기"},
            {"pt_BR", "Abrir Pasta Após Exportar"},
            {"ru",    "Открывать папку после экспорта"},
            {"th",    "เปิดโฟลเดอร์อัตโนมัติหลังส่งออก"},
            {"tr",    "Dışa Aktarmadan Sonra Klasörü Aç"},
            {"uk",    "Відкривати папку після експорту"},
            {"vi",    "Tự động mở thư mục sau khi xuất"}
        }},
        {"EXPORT_OPT_OPEN_FOLDER_DESC", {
            {"zh_CN", "在生成地图图片后自动打开导出图片所在的文件夹窗口；不勾选则不会打开。"},
            {"zh_TW", "在生成地圖圖片後自動打開匯出圖片所在的資料夾視窗；不勾選則不會打開。"},
            {"en_US", "Automatically open the folder containing the generated map image(s) after export. Uncheck to disable."},
            {"de",    "Öffnet nach dem Erstellen der Kartenbilder automatisch das Zielverzeichnis. Deaktivieren, um nicht zu öffnen."},
            {"es",    "Abre automáticamente la carpeta donde se guardaron las imágenes del mapa tras la exportación."},
            {"fr",    "Ouvre automatiquement le dossier contenant les images de la carte après l'exportation."},
            {"id",    "Buka otomatis jendela folder tempat gambar peta disimpan setelah selesai dibuat."},
            {"it",    "Apre automaticamente la finestra della cartella contenente le immagini della mappa generate."},
            {"ja",    "マップ画像の生成完了後に保存先フォルダのウィンドウを自動的に開きます。チェックを外すと開きません。"},
            {"ko",    "지도 이미지 생성 후 저장된 폴더 창을 자동으로 엽니다. 체크를 해제하면 열리지 않습니다."},
            {"pt_BR", "Abre automaticamente a janela da pasta onde as imagens do mapa foram geradas."},
            {"ru",    "Автоматически открывать окно папки с созданными изображениями карты после завершения экспорта."},
            {"th",    "เปิดหน้าต่างโฟลเดอร์ที่เก็บรูปภาพแผนที่โดยอัตโนมัติหลังจากสร้างเสร็จ หากไม่เลือกจะไม่เปิด"},
            {"tr",    "Harita görüntüleri oluşturulduktan sonra bulunduğu klasör penceresini otomatik olarak açar."},
            {"uk",    "Автоматично відкривати вікно папки зі створеними зображеннями мапи після завершення експорту."},
            {"vi",    "Tự động mở cửa sổ thư mục chứa ảnh bản đồ sau khi tạo xong; bỏ chọn để không mở."}
        }},
        {"EXPORT_OPT_MAX_SIZE", {
            {"zh_CN", "单张图像最大尺寸"},
            {"zh_TW", "單張圖像最大尺寸"},
            {"en_US", "Max Single Image Size"},
            {"de",    "Max. Einzelbildgröße"},
            {"es",    "Tamaño máx. de imagen única"},
            {"fr",    "Taille max. d'une image"},
            {"id",    "Ukuran Maks. Gambar Tunggal"},
            {"it",    "Dimensione max immagine singola"},
            {"ja",    "単一画像の最大サイズ"},
            {"ko",    "단일 이미지 최대 크기"},
            {"pt_BR", "Tamanho Máx. de Imagem Única"},
            {"ru",    "Макс. размер одного изображения"},
            {"th",    "ขนาดรูปภาพเดี่ยวสูงสุด"},
            {"tr",    "Maks. Tek Görüntü Boyutu"},
            {"uk",    "Макс. розмір одного зображення"},
            {"vi",    "Kích thước ảnh đơn tối đa"}
        }},
        {"EXPORT_OPT_MAX_SIZE_VAL", {
            {"zh_CN", "%dx%d 区域"},
            {"zh_TW", "%dx%d 區域"},
            {"en_US", "%dx%d reg"},
            {"de",    "%dx%d Reg."},
            {"es",    "%dx%d reg"},
            {"fr",    "%dx%d rég"},
            {"id",    "%dx%d reg"},
            {"it",    "%dx%d reg"},
            {"ja",    "%dx%d リージョン"},
            {"ko",    "%dx%d 구역"},
            {"pt_BR", "%dx%d reg"},
            {"ru",    "%dx%d рег."},
            {"th",    "%dx%d โซน"},
            {"tr",    "%dx%d bölge"},
            {"uk",    "%dx%d рег."},
            {"vi",    "%dx%d vùng"}
        }},
        {"EXPORT_OPT_MAX_SIZE_UNSCALED", {
            {"zh_CN", "不缩放"},
            {"zh_TW", "不縮放"},
            {"en_US", "Unscaled"},
            {"de",    "Unskaliert"},
            {"es",    "Sin escalar"},
            {"fr",    "Sans échelle"},
            {"id",    "Tanpa Skala"},
            {"it",    "Non ridimensionato"},
            {"ja",    "縮小なし"},
            {"ko",    "원본 크기"},
            {"pt_BR", "Sem Escala"},
            {"ru",    "Без сжатия"},
            {"th",    "ไม่ย่อขนาด"},
            {"tr",    "Ölçeklenmemiş"},
            {"uk",    "Без масштабування"},
            {"vi",    "Nguyên gốc"}
        }},
        {"EXPORT_OPT_MAX_SIZE_DESC", {
            {"zh_CN", "以区域（Region，512x512像素）为单位的正方形等效最大分辨率，超出时导出的完整图像将按比例缩小。例如 20x20 为 400 个区域 = 10240x10240 像素。导出的图像不一定是正方形，10240x10240 也等效于 5120x20480。"},
            {"zh_TW", "以區域（Region，512x512像素）為單位的正方形等效最大解析度，超出時匯出的完整圖像將按比例縮小。例如 20x20 為 400 個區域 = 10240x10240 像素。匯出的圖像不一定是正方形，10240x10240 也等效於 5120x20480。"},
            {"en_US", "The size in regions of a square image equivalent to the maximum resolution that a full exported image will be scaled down to, if necessary. For example, 20x20 is 400 regions = 10240x10240 pixels. The exported image doesn't need to be a square. 10240x10240 is also 5120x20480."},
            {"de",    "Die Größe in Regionen eines quadratischen Bildes entsprechend der maximalen Auflösung, auf die ein exportiertes Bild bei Bedarf herunterskaliert wird (z.B. 20x20 = 400 Regionen = 10240x10240 Pixel)."},
            {"es",    "El tamaño en regiones de una imagen cuadrada equivalente a la resolución máxima a la que se reducirá la imagen exportada si es necesario (p. ej., 20x20 = 400 regiones = 10240x10240 píxeles)."},
            {"fr",    "La taille en régions d'une image carrée équivalente à la résolution maximale à laquelle l'image exportée sera réduite si nécessaire (ex. 20x20 = 400 régions = 10240x10240 pixels)."},
            {"id",    "Ukuran dalam wilayah (region) dari gambar persegi yang setara dengan resolusi maksimum gambar ekspor sebelum diperkecil bila perlu (misal 20x20 = 400 wilayah = 10240x10240 piksel)."},
            {"it",    "La dimensione in regioni di un'immagine quadrata equivalente alla risoluzione massima a cui verrà ridotta l'immagine esportata se necessario (es. 20x20 = 400 regioni = 10240x10240 pixel)."},
            {"ja",    "必要に応じてエクスポート画像を縮小する際の最大解像度に相当する正方形リージョン数です（例：20x20＝400リージョン＝10240x10240ピクセル）。出力画像は正方形である必要はありません。"},
            {"ko",    "필요 시 내보낸 전체 이미지를 축소할 최대 해상도에 해당하는 정사각형 구역(Region) 크기입니다. 예: 20x20은 400구역 = 10240x10240 픽셀입니다."},
            {"pt_BR", "O tamanho em regiões de uma imagem quadrada equivalente à resolução máxima para a qual a imagem exportada será reduzida, se necessário (ex: 20x20 = 400 regiões = 10240x10240 pixels)."},
            {"ru",    "Размер в регионах квадратного изображения, эквивалентный максимальному разрешению, до которого при необходимости будет уменьшено изображение (например, 20x20 = 400 регионов = 10240x10240 пикселей)."},
            {"th",    "ขนาดเป็นโซนของภาพสี่เหลี่ยมจัตุรัสที่เทียบเท่ากับความละเอียดสูงสุดที่จะย่อขนาดภาพส่งออกหากจำเป็น (เช่น 20x20 คือ 400 โซน = 10240x10240 พิกเซล)"},
            {"tr",    "Gerekirse dışa aktarılan görüntünün küçültüleceği maksimum çözünürlüğe eşdeğer kare bölge boyutu (örneğin 20x20 = 400 bölge = 10240x10240 piksel)."},
            {"uk",    "Розмір у регіонах квадратного зображення, еквівалентний максимальній роздільній здатності, до якої за потреби буде зменшено зображення (наприклад, 20x20 = 400 регіонів = 10240x10240 пікселів)."},
            {"vi",    "Kích thước tính theo vùng (region) của ảnh vuông tương đương với độ phân giải tối đa mà ảnh xuất ra sẽ được thu nhỏ nếu cần (ví dụ: 20x20 = 400 vùng = 10240x10240 pixel)."}
        }},
        {"EXPORT_ON", {
            {"zh_CN", "开"},
            {"zh_TW", "開"},
            {"en_US", "On"},
            {"de",    "Ein"},
            {"es",    "Sí"},
            {"fr",    "Oui"},
            {"id",    "Aktif"},
            {"it",    "Sì"},
            {"ja",    "オン"},
            {"ko",    "켜짐"},
            {"pt_BR", "Ligado"},
            {"ru",    "Вкл"},
            {"th",    "เปิด"},
            {"tr",    "Açık"},
            {"uk",    "Увімк."},
            {"vi",    "Bật"}
        }},
        {"EXPORT_OFF", {
            {"zh_CN", "关"},
            {"zh_TW", "關"},
            {"en_US", "Off"},
            {"de",    "Aus"},
            {"es",    "No"},
            {"fr",    "Non"},
            {"id",    "Nonaktif"},
            {"it",    "No"},
            {"ja",    "オフ"},
            {"ko",    "꺼짐"},
            {"pt_BR", "Desligado"},
            {"ru",    "Выкл"},
            {"th",    "ปิด"},
            {"tr",    "Kapalı"},
            {"uk",    "Вимк."},
            {"vi",    "Tắt"}
        }},
        {"EXPORT_CONFIRM", {
            {"zh_CN", "确认"},
            {"zh_TW", "確認"},
            {"en_US", "Confirm"},
            {"de",    "Bestätigen"},
            {"es",    "Confirmar"},
            {"fr",    "Confirmer"},
            {"id",    "Konfirmasi"},
            {"it",    "Conferma"},
            {"ja",    "確認"},
            {"ko",    "확인"},
            {"pt_BR", "Confirmar"},
            {"ru",    "Подтвердить"},
            {"th",    "ยืนยัน"},
            {"tr",    "Onayla"},
            {"uk",    "Підтвердити"},
            {"vi",    "Xác nhận"}
        }},
        {"EXPORT_BACK", {
            {"zh_CN", "返回"},
            {"zh_TW", "返回"},
            {"en_US", "Back"},
            {"de",    "Zurück"},
            {"es",    "Volver"},
            {"fr",    "Retour"},
            {"id",    "Kembali"},
            {"it",    "Indietro"},
            {"ja",    "戻る"},
            {"ko",    "뒤로"},
            {"pt_BR", "Voltar"},
            {"ru",    "Назад"},
            {"th",    "กลับ"},
            {"tr",    "Geri"},
            {"uk",    "Назад"},
            {"vi",    "Quay lại"}
        }},
        {"EXPORT_EXPORTING", {
            {"zh_CN", "正在导出..."},
            {"zh_TW", "正在匯出..."},
            {"en_US", "Exporting..."},
            {"de",    "Exportiere..."},
            {"es",    "Exportando..."},
            {"fr",    "Exportation en cours..."},
            {"id",    "Mengekspor..."},
            {"it",    "Esportazione in corso..."},
            {"ja",    "エクスポート中..."},
            {"ko",    "내보내는 중..."},
            {"pt_BR", "Exportando..."},
            {"ru",    "Экспорт..."},
            {"th",    "กำลังส่งออก..."},
            {"tr",    "Dışa aktarılıyor..."},
            {"uk",    "Експортування..."},
            {"vi",    "Đang xuất..."}
        }},
        {"EXPORT_RES_SUCCESS", {
            {"zh_CN", "地图已成功导出！"},
            {"zh_TW", "地圖已成功匯出！"},
            {"en_US", "Successfully exported the map!"},
            {"de",    "Karte erfolgreich exportiert!"},
            {"es",    "¡Mapa exportado con éxito!"},
            {"fr",    "Carte exportée avec succès !"},
            {"id",    "Berhasil mengekspor peta!"},
            {"it",    "Mappa esportata con successo!"},
            {"ja",    "マップのエクスポートに成功しました！"},
            {"ko",    "지도를 성공적으로 내보냈습니다!"},
            {"pt_BR", "Mapa exportado com sucesso!"},
            {"ru",    "Карта успешно экспортирована!"},
            {"th",    "ส่งออกแผนที่สำเร็จแล้ว!"},
            {"tr",    "Harita başarıyla dışa aktarıldı!"},
            {"uk",    "Мапу успішно експортовано!"},
            {"vi",    "Đã xuất bản đồ thành công!"}
        }},
        {"EXPORT_RES_EMPTY", {
            {"zh_CN", "导出的区域为空！"},
            {"zh_TW", "匯出的區域為空！"},
            {"en_US", "The exported area is empty!"},
            {"de",    "Der exportierte Bereich ist leer!"},
            {"es",    "¡El área exportada está vacía!"},
            {"fr",    "La zone exportée est vide !"},
            {"id",    "Area yang diekspor kosong!"},
            {"it",    "L'area esportata è vuota!"},
            {"ja",    "エクスポート対象領域が空です！"},
            {"ko",    "내보낼 영역이 비어 있습니다!"},
            {"pt_BR", "A área exportada está vazia!"},
            {"ru",    "Экспортируемая область пуста!"},
            {"th",    "พื้นที่ที่ส่งออกว่างเปล่า!"},
            {"tr",    "Dışa aktarılan alan boş!"},
            {"uk",    "Експортована область порожня!"},
            {"vi",    "Vùng xuất ra trống!"}
        }},
        {"EXPORT_RES_NOT_PREPARED", {
            {"zh_CN", "地图尚未准备就绪，无法导出。"},
            {"zh_TW", "地圖尚未準備就緒，無法匯出。"},
            {"en_US", "Can't export while the map hasn't been prepared yet."},
            {"de",    "Export nicht möglich, solange die Karte noch nicht bereit ist."},
            {"es",    "No se puede exportar mientras el mapa aún no está preparado."},
            {"fr",    "Impossible d'exporter tant que la carte n'est pas prête."},
            {"id",    "Tidak dapat mengekspor saat peta belum siap."},
            {"it",    "Impossibile esportare finché la mappa non è pronta."},
            {"ja",    "マップの準備が完了していないためエクスポートできません。"},
            {"ko",    "지도가 아직 준비되지 않아 내보낼 수 없습니다."},
            {"pt_BR", "Não é possível exportar enquanto o mapa não estiver pronto."},
            {"ru",    "Невозможно экспортировать, пока карта еще не подготовлена."},
            {"th",    "ไม่สามารถส่งออกได้ในขณะที่แผนที่ยังไม่พร้อม"},
            {"tr",    "Harita henüz hazır değilken dışa aktarılamaz."},
            {"uk",    "Неможливо експортувати, поки мапу ще не підготовлено."},
            {"vi",    "Không thể xuất khi bản đồ chưa sẵn sàng."}
        }},
        {"EXPORT_RES_TOO_BIG", {
            {"zh_CN", "导出区域过大，无法缩放！请选择较小的区域或使用多张无缩放图像模式。"},
            {"zh_TW", "匯出區域過大，無法縮放！請選擇較小的區域或使用多張無縮放圖像模式。"},
            {"en_US", "The exported area is too big to scale down! Please choose a smaller area."},
            {"de",    "Der exportierte Bereich ist zu groß zum Skalieren! Bitte wähle einen kleineren Bereich."},
            {"es",    "¡El área exportada es demasiado grande para reducirla! Elige un área más pequeña."},
            {"fr",    "La zone exportée est trop grande ! Veuillez choisir une zone plus petite."},
            {"id",    "Area yang diekspor terlalu besar untuk diperkecil! Silakan pilih area yang lebih kecil."},
            {"it",    "L'area esportata è troppo grande! Scegli un'area più piccola."},
            {"ja",    "エクスポート領域が大きすぎます！より小さいサイズを選択してください。"},
            {"ko",    "내보낼 영역이 너무 커서 축소할 수 없습니다! 더 작은 크기를 선택하세요."},
            {"pt_BR", "A área exportada é muito grande para reduzir! Escolha uma área menor."},
            {"ru",    "Экспортируемая область слишком велика! Пожалуйста, выберите меньший размер."},
            {"th",    "พื้นที่ที่ส่งออกใหญ่เกินไปที่จะย่อขนาด! โปรดเลือกขนาดที่เล็กลง"},
            {"tr",    "Dışa aktarılan alan küçültmek için çok büyük! Lütfen daha küçük bir alan seçin."},
            {"uk",    "Експортована область занадто велика! Будь ласка, оберіть менший розмір."},
            {"vi",    "Vùng xuất ra quá lớn để thu nhỏ! Vui lòng chọn kích thước nhỏ hơn."}
        }},
        {"EXPORT_RES_OOM", {
            {"zh_CN", "内存不足导致导出失败！请重试或调小单张图像最大尺寸。"},
            {"zh_TW", "記憶體不足導致匯出失敗！請重試或調小單張圖像最大尺寸。"},
            {"en_US", "The export failed because the Java heap ran out of memory! Please try again or adhere to a smaller max size."},
            {"de",    "Export wegen Speichermangel fehlgeschlagen! Bitte versuche es mit einer kleineren Maximalgröße erneut."},
            {"es",    "¡La exportación falló por falta de memoria! Inténtalo de nuevo con un tamaño máximo menor."},
            {"fr",    "L'exportation a échoué par manque de mémoire ! Réessayez avec une taille maximale inférieure."},
            {"id",    "Ekspor gagal karena kehabisan memori! Silakan coba lagi dengan ukuran maksimum yang lebih kecil."},
            {"it",    "Esportazione fallita per memoria insufficiente! Riprova con una dimensione massima inferiore."},
            {"ja",    "メモリ不足のためエクスポートに失敗しました！最大サイズを小さくして再試行してください。"},
            {"ko",    "메모리 부족으로 내보내기에 실패했습니다! 최대 크기를 줄여서 다시 시도하세요."},
            {"pt_BR", "A exportação falhou por falta de memória! Tente novamente com um tamanho máximo menor."},
            {"ru",    "Ошибка экспорта из-за нехватки памяти! Попробуйте снова или уменьшите макс. размер."},
            {"th",    "การส่งออกล้มเหลวเนื่องจากหน่วยความจำไม่เพียงพอ! โปรดลองอีกครั้งหรือลดขนาดสูงสุดลง"},
            {"tr",    "Bellek yetersizliği nedeniyle dışa aktarma başarısız oldu! Lütfen daha küçük bir maksimum boyut deneyin."},
            {"uk",    "Помилка експорту через брак пам'яті! Спробуйте ще раз або зменшіть макс. розмір."},
            {"vi",    "Xuất thất bại do thiếu bộ nhớ! Vui lòng thử lại hoặc giảm kích thước tối đa."}
        }},
        {"EXPORT_RES_IO", {
            {"zh_CN", "发生读写异常（IO Exception），导出失败！"},
            {"zh_TW", "發生讀寫異常（IO Exception），匯出失敗！"},
            {"en_US", "The export failed because of an IO exception!"},
            {"de",    "Der Export ist aufgrund eines E/A-Fehlers fehlgeschlagen!"},
            {"es",    "¡La exportación falló debido a un error de E/S!"},
            {"fr",    "L'exportation a échoué en raison d'une erreur d'E/S !"},
            {"id",    "Ekspor gagal karena kesalahan I/O!"},
            {"it",    "L'esportazione è fallita a causa di un errore di I/O!"},
            {"ja",    "I/O エラーのためエクスポートに失敗しました！"},
            {"ko",    "입출력(I/O) 오류로 인해 내보내기에 실패했습니다!"},
            {"pt_BR", "A exportação falhou devido a um erro de E/S!"},
            {"ru",    "Экспорт не удался из-за ошибки ввода-вывода (I/O)!"},
            {"th",    "การส่งออกล้มเหลวเนื่องจากข้อผิดพลาดในการอ่าน/เขียนไฟล์ (I/O)!"},
            {"tr",    "Bir G/Ç (I/O) hatası nedeniyle dışa aktarma başarısız oldu!"},
            {"uk",    "Експорт не вдався через помилку введення-виведення (I/O)!"},
            {"vi",    "Xuất thất bại do lỗi đọc/ghi tệp (I/O)!"}
        }},
        {"EXPORT_PROGRESS", {
            {"zh_CN", "导出进度"},
            {"zh_TW", "匯出進度"},
            {"en_US", "Export Progress"},
            {"de",    "Export-Fortschritt"},
            {"es",    "Progreso de exportación"},
            {"fr",    "Progression de l'export"},
            {"id",    "Kemajuan Ekspor"},
            {"it",    "Avanzamento esportazione"},
            {"ja",    "エクスポート進行状況"},
            {"ko",    "내보내기 진행률"},
            {"pt_BR", "Progresso da Exportação"},
            {"ru",    "Прогресс экспорта"},
            {"th",    "ความคืบหน้าการส่งออก"},
            {"tr",    "Dışa Aktarma İlerlemesi"},
            {"uk",    "Прогрес експорту"},
            {"vi",    "Tiến trình xuất"}
        }},
        {"EXPORT_CANCEL_AND_CLEAR", {
            {"zh_CN", "终止并清空"},
            {"zh_TW", "終止並清空"},
            {"en_US", "Cancel & Clear"},
            {"de",    "Abbrechen & Bereinigen"},
            {"es",    "Cancelar y limpiar"},
            {"fr",    "Annuler et effacer"},
            {"id",    "Batalkan & Bersihkan"},
            {"it",    "Annulla e cancella"},
            {"ja",    "中断して消去"},
            {"ko",    "중단 및 정리"},
            {"pt_BR", "Cancelar e Limpar"},
            {"ru",    "Отменить и очистить"},
            {"th",    "ยกเลิกและล้างข้อมูล"},
            {"tr",    "İptal Et ve Temizle"},
            {"uk",    "Скасувати та очистити"},
            {"vi",    "Hủy và xóa"}
        }},
        {"EXPORT_RES_CANCELED", {
            {"zh_CN", "导出已终止，已清空刚刚生成的图片。"},
            {"zh_TW", "匯出已終止，已清空剛剛生成的圖片。"},
            {"en_US", "Export was cancelled and generated images have been cleared."},
            {"de",    "Export wurde abgebrochen und erstellte Bilder wurden gelöscht."},
            {"es",    "Exportación cancelada y se han eliminado las imágenes generadas."},
            {"fr",    "Exportation annulée et les images générées ont été supprimées."},
            {"id",    "Ekspor dibatalkan dan gambar yang baru dibuat telah dibersihkan."},
            {"it",    "Esportazione annullata e le immagini generate sono state rimosse."},
            {"ja",    "エクスポートが中断され、生成された画像は消去されました。"},
            {"ko",    "내보내기가 중단되었으며 방금 생성된 이미지가 정리되었습니다."},
            {"pt_BR", "Exportação cancelada e as imagens geradas foram limpas."},
            {"ru",    "Экспорт отменен, только что созданные изображения удалены."},
            {"th",    "การส่งออกถูกยกเลิกและรูปภาพที่เพิ่งสร้างได้รับการล้างแล้ว"},
            {"tr",    "Dışa aktarma iptal edildi ve oluşturulan görüntüler temizlendi."},
            {"uk",    "Експорт скасовано, щойно створені зображення очищено."},
            {"vi",    "Quá trình xuất đã bị hủy và các ảnh vừa tạo đã được xóa."}
        }},
        {"HOTKEY_TOGGLE_SEEDMAP", {
            {"zh_CN", "开关种子地图"},
            {"zh_TW", "開關種子地圖"},
            {"en_US", "Toggle Seed Map"},
            {"de",    "Saatgutkarte umschalten"},
            {"es",    "Alternar mapa de semillas"},
            {"fr",    "Basculer la carte de graines"},
            {"id",    "Beralih Peta Benih"},
            {"it",    "Attiva/disattiva mappa semi"},
            {"ja",    "シードマップの切り替え"},
            {"ko",    "시드 지도 전환"},
            {"pt_BR", "Alternar Mapa de Sementes"},
            {"ru",    "Переключить карту сида"},
            {"th",    "สลับแผนที่เมล็ดพันธุ์"},
            {"tr",    "Tohum Haritasını Aç/Kapat"},
            {"uk",    "Перемкнути карту сідів"},
            {"vi",    "Bật/tắt Bản đồ Hạt giống"}
        }},
        {"HOTKEY_OPEN_DEATH_MGR", {
            {"zh_CN", "开关死亡记录"},
            {"zh_TW", "開關死亡紀錄"},
            {"en_US", "Toggle Death Records"},
            {"de",    "Todesaufzeichnungen umschalten"},
            {"es",    "Alternar registros de muerte"},
            {"fr",    "Basculer les enregistrements de mort"},
            {"id",    "Beralih Catatan Kematian"},
            {"it",    "Attiva/disattiva record di morte"},
            {"ja",    "死亡記録の切り替え"},
            {"ko",    "사망 기록 전환"},
            {"pt_BR", "Alternar Registros de Morte"},
            {"ru",    "Переключить записи смертей"},
            {"th",    "สลับบันทึกการตาย"},
            {"tr",    "Ölüm Kayıtlarını Aç/Kapat"},
            {"uk",    "Перемкнути записи смертей"},
            {"vi",    "Bật/tắt Nhật ký Tử vong"}
        }}
    };

    static std::filesystem::path GetLanguageDirectory() {
        // 1. 优先通过模块句柄获取 ChiyanMap.dll 所在的绝对路径下的 lang 目录
        // 彻底免疫不同启动器/游戏工作路径 (CWD) 差异导致的相对路径失效
        HMODULE hMod = NULL;
        if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCWSTR)&Init, &hMod)) {
            wchar_t modPath[MAX_PATH];
            if (GetModuleFileNameW(hMod, modPath, MAX_PATH)) {
                std::filesystem::path dllDir = std::filesystem::path(modPath).parent_path();
                auto langDir = dllDir / "lang";
                std::error_code ec;
                if (std::filesystem::exists(langDir, ec) && std::filesystem::is_directory(langDir, ec)) {
                    return langDir;
                }
            }
        }

        // 2. 尝试 LeviLamina API 获取的 lang 目录
        try {
            auto dir = chiyan_map::ChiyanMap::getInstance().getSelf().getLangDir();
            std::error_code ec;
            if (std::filesystem::exists(dir, ec) && std::filesystem::is_directory(dir, ec)) {
                return dir;
            }
        } catch (...) {}

        // 3. 常见 Fallbacks
        std::error_code ec;
        if (std::filesystem::exists("mods/ChiyanMap/lang", ec)) {
            return "mods/ChiyanMap/lang";
        }
        if (std::filesystem::exists("plugins/ChiyanMap/lang", ec)) {
            return "plugins/ChiyanMap/lang";
        }
        if (std::filesystem::exists("lang", ec)) {
            return "lang";
        }
        return "mods/ChiyanMap/lang";
    }

    void Init() {
        g_langDir = GetLanguageDirectory();

        // 1. 优先尝试 LeviLamina 官方 I18n 批量加载
        if (auto res = ll::i18n::getInstance().load(g_langDir); !res) {
            if (g_langDir != "lang" && std::filesystem::exists("lang")) {
                (void)ll::i18n::getInstance().load("lang");
            }
        }

        // 2. 安全逐文件加载并注册至 ll::i18n，防止目录批量加载中某文件解析异常导致后续语言包漏载
        try {
            std::error_code ec;
            if (std::filesystem::exists(g_langDir, ec) && std::filesystem::is_directory(g_langDir, ec)) {
                for (const auto& entry : std::filesystem::directory_iterator(g_langDir, ec)) {
                    if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                        std::string stem = entry.path().stem().string();
                        try {
                            std::ifstream ifs(entry.path());
                            if (ifs.is_open()) {
                                json j;
                                ifs >> j;
                                for (auto& [k, v] : j.items()) {
                                    if (v.is_string()) {
                                        ll::i18n::getInstance().set(stem, k, v.get<std::string>());
                                    }
                                }
                            }
                        } catch (...) {}
                    }
                }
            }
        } catch (...) {}

        // 3. 注册内置多语言词条至 ll::i18n (提供强保底)
        for (const auto& [k, transMap] : g_builtinTranslations) {
            for (const auto& [lang, text] : transMap) {
                ll::i18n::getInstance().set(lang, k, text);
            }
        }

        ScanLanguages();
        LoadConfig();
    }

    void ScanLanguages() {
        g_availableLanguages.clear();

        static const std::vector<std::pair<std::string, std::string>> orderedLangs = {
            {"zh_CN", "简体中文"},
            {"zh_TW", "繁體中文"},
            {"en_US", "English"},
            {"de", "Deutsch"},
            {"es", "Español"},
            {"fr", "Français"},
            {"id", "Bahasa Indonesia"},
            {"it", "Italiano"},
            {"ja", "日本語"},
            {"ko", "한국어"},
            {"pt_BR", "Português (Brasil)"},
            {"ru", "Русский"},
            {"th", "ไทย"},
            {"tr", "Türkçe"},
            {"uk", "Українська"},
            {"vi", "Tiếng Việt"}
        };

        try {
            std::error_code ec;
            if (std::filesystem::exists(g_langDir, ec) && std::filesystem::is_directory(g_langDir, ec)) {
                // 先按照推荐顺序添加已知语言
                for (const auto& item : orderedLangs) {
                    auto p = g_langDir / (item.first + ".json");
                    if (std::filesystem::exists(p, ec)) {
                        g_availableLanguages.push_back(item);
                    }
                }

                // 再扫描并补充其余第三方/自制语言包
                for (const auto& entry : std::filesystem::directory_iterator(g_langDir, ec)) {
                    if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                        std::string stem = entry.path().stem().string();
                        bool exists = false;
                        for (const auto& p : g_availableLanguages) {
                            if (p.first == stem) { exists = true; break; }
                        }
                        if (!exists) {
                            g_availableLanguages.push_back({stem, stem});
                        }
                    }
                }
            }
        } catch (...) {}

        if (g_availableLanguages.empty()) {
            g_availableLanguages.push_back({"en_US", "English"});
        }
    }

    void LoadLanguage(const std::string& langCode) {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        g_currentLanguage = langCode;
        g_translationCache.clear();
    }

    void LoadConfig() {
        std::string filePath = "mods/ChiyanMap/config.json";
        if (!std::filesystem::exists(filePath)) {
            LANGID langId = GetUserDefaultUILanguage();
            WORD primary = PRIMARYLANGID(langId);
            if (primary == LANG_CHINESE) {
                WORD sub = (WORD)(langId & 0x3ff);
                if (sub == 0x0404 || sub == 0x0c04 || sub == 0x1404) {
                    g_currentLanguage = "zh_TW";
                } else {
                    g_currentLanguage = "zh_CN";
                }
            }
            else if (primary == LANG_GERMAN) g_currentLanguage = "de";
            else if (primary == LANG_FRENCH) g_currentLanguage = "fr";
            else if (primary == LANG_INDONESIAN) g_currentLanguage = "id";
            else if (primary == LANG_ITALIAN) g_currentLanguage = "it";
            else if (primary == LANG_JAPANESE) g_currentLanguage = "ja";
            else if (primary == LANG_KOREAN) g_currentLanguage = "ko";
            else if (primary == LANG_PORTUGUESE) g_currentLanguage = "pt_BR";
            else if (primary == LANG_RUSSIAN) g_currentLanguage = "ru";
            else if (primary == LANG_THAI) g_currentLanguage = "th";
            else if (primary == LANG_TURKISH) g_currentLanguage = "tr";
            else if (primary == LANG_UKRAINIAN) g_currentLanguage = "uk";
            else if (primary == LANG_VIETNAMESE) g_currentLanguage = "vi";
            else if (primary == LANG_SPANISH) g_currentLanguage = "es";
            else g_currentLanguage = "en_US";

            SaveConfig();
            LoadLanguage(g_currentLanguage);
            return;
        }

        std::ifstream in(filePath);
        if (in.is_open()) {
            try {
                json j;
                in >> j;
                g_currentLanguage = j.value("language", "en_US");
                if (g_currentLanguage == "en") {
                    g_currentLanguage = "en_US";
                }
                MapRenderState::showMiniMap = j.value("showMiniMap", true);
                MapRenderState::isSquareMap = j.value("isSquareMap", false);
                MapRenderState::rotateMiniMap = j.value("rotateMiniMap", false);
                MapRenderState::uiTextScale = j.value("uiTextScale", 1.0f);
                MapRenderState::miniMapScale = j.value("miniMapScale", 1.0f);
                MapRenderState::miniMapOffsetX = j.value("miniMapOffsetX", 0.0f);
                MapRenderState::miniMapOffsetY = j.value("miniMapOffsetY", 0.0f);
                MapRenderState::showWaypointsOnMinimap = j.value("showWaypointsOnMinimap", true);
                MapRenderState::showRadar = j.value("showRadar", true);
                MapRenderState::bigMapShowEntities = j.value("bigMapShowEntities", false);
                MapRenderState::bigMapShowMarkers = j.value("bigMapShowMarkers", true);
                MapRenderState::showChunkGrid = j.value("showChunkGrid", false);
                MapRenderState::g_caveModeType = j.value("caveModeType", (int)MapRenderState::CaveModeType::Layered);
                MapRenderState::g_caveTopYAuto = j.value("caveTopYAuto", true);
                MapRenderState::g_caveTopY = j.value("caveTopY", 64);
                MapRenderState::g_caveDepth = j.value("caveDepth", 30);
                MapRenderState::g_legibleCaveMaps = j.value("legibleCaveMaps", false);
                MapRenderState::exportForceFullMap = j.value("exportForceFullMap", false);
                MapRenderState::exportMultipleImages = j.value("exportMultipleImages", false);
                MapRenderState::exportOpenFolder = j.value("exportOpenFolder", true);
                MapRenderState::exportScaleDownSquare = std::clamp(j.value("exportScaleDownSquare", 20), 0, 90);
                // 读取快捷键绑定 (持久化保存，兼容旧版整数配置与新版组合键对象配置)
                // openBigMap 支持自定义按键，但不可为空；若配置中为空则自动保底为默认 M 键 (0x4D)
                if (j.contains("hotkeys") && j["hotkeys"].is_object()) {
                    auto const& hk = j["hotkeys"];
                    auto def = MapRenderState::HotkeyBindings::Defaults();

                    auto loadHk = [&](const char* name, const MapRenderState::Hotkey& defVal) -> MapRenderState::Hotkey {
                        if (!hk.contains(name)) return defVal;
                        const auto& val = hk[name];
                        if (val.is_number_integer()) {
                            MapRenderState::Hotkey h;
                            h.key = val.get<int>();
                            h.modifiers = 0;
                            return h;
                        }
                        if (val.is_object()) {
                            MapRenderState::Hotkey h;
                            h.key = val.value("key", defVal.key);
                            h.modifiers = val.value("modifiers", defVal.modifiers);
                            if (val.value("ctrl", false))  h.modifiers |= MapRenderState::Hotkey::HK_MOD_CTRL;
                            if (val.value("shift", false)) h.modifiers |= MapRenderState::Hotkey::HK_MOD_SHIFT;
                            if (val.value("alt", false))   h.modifiers |= MapRenderState::Hotkey::HK_MOD_ALT;
                            return h;
                        }
                        return defVal;
                    };

                    MapRenderState::g_hotkeys.openBigMap       = loadHk("openBigMap", def.openBigMap);
                    if (MapRenderState::g_hotkeys.openBigMap.IsEmpty()) {
                        MapRenderState::g_hotkeys.openBigMap = def.openBigMap;
                    }
                    MapRenderState::g_hotkeys.openWaypointMgr   = loadHk("openWaypointMgr", def.openWaypointMgr);
                    MapRenderState::g_hotkeys.openDeathPointMgr = loadHk("openDeathPointMgr", def.openDeathPointMgr);
                    MapRenderState::g_hotkeys.toggleMinimap     = loadHk("toggleMinimap", def.toggleMinimap);
                    MapRenderState::g_hotkeys.toggleMinimapShape = loadHk("toggleMinimapShape", def.toggleMinimapShape);
                    MapRenderState::g_hotkeys.toggleMinimapRot = loadHk("toggleMinimapRot", def.toggleMinimapRot);
                    MapRenderState::g_hotkeys.holdEntities     = loadHk("holdEntities", def.holdEntities);
                    MapRenderState::g_hotkeys.toggleSeedMap    = loadHk("toggleSeedMap", def.toggleSeedMap);
                }

                auto seedMapSettings = SeedMapManager::GetSettings();
                if (j.contains("seedMap") && j["seedMap"].is_object()) {
                    const auto& seedMap = j["seedMap"];
                    seedMapSettings.enabled = seedMap.value("enabled", seedMapSettings.enabled);
                    seedMapSettings.searchMode = seedMap.value("searchMode", 0) == 1
                        ? SeedMapManager::SearchMode::Manual
                        : SeedMapManager::SearchMode::VisibleMap;
                    if (const auto dimension = SeedMapManager::DimensionFromGameDimensionId(
                            seedMap.value("dimension", static_cast<int>(seedMapSettings.targetDimension))
                        )) {
                        seedMapSettings.targetDimension = *dimension;
                    }
                    seedMapSettings.manualCenterX = seedMap.value("manualCenterX", seedMapSettings.manualCenterX);
                    seedMapSettings.manualCenterZ = seedMap.value("manualCenterZ", seedMapSettings.manualCenterZ);
                    seedMapSettings.manualRadius = seedMap.value("manualRadius", seedMapSettings.manualRadius);
                    if (seedMap.contains("enabledLayers") && seedMap["enabledLayers"].is_array()) {
                        const auto& layers = seedMap["enabledLayers"];
                        const std::size_t layerCount = std::min(layers.size(), seedMapSettings.enabledLayers.size());
                        for (std::size_t index = 0; index < layerCount; ++index) {
                            if (layers[index].is_boolean()) {
                                seedMapSettings.enabledLayers[index] = layers[index].get<bool>();
                            }
                        }
                    }
                    SeedMapManager::SetSettings(seedMapSettings);
                }

                if (j.contains("manualSeeds") && j["manualSeeds"].is_object()) {
                    std::unordered_map<std::string, std::uint64_t> manualSeeds;
                    for (auto& [wId, val] : j["manualSeeds"].items()) {
                        try {
                            if (val.is_string()) {
                                manualSeeds[wId] = std::stoull(val.get<std::string>());
                            } else if (val.is_number_unsigned()) {
                                manualSeeds[wId] = val.get<std::uint64_t>();
                            }
                        } catch(...) {}
                    }
                    SeedMapManager::SetSavedManualSeeds(manualSeeds);
                }
            } catch (...) {
                g_currentLanguage = "en_US";
            }
            in.close();
        }
        LoadLanguage(g_currentLanguage);
    }

    void SaveConfig() {
        std::string filePath = "mods/ChiyanMap/config.json";
        json j;
        j["language"] = g_currentLanguage;
        j["showMiniMap"] = MapRenderState::showMiniMap;
        j["isSquareMap"] = MapRenderState::isSquareMap;
        j["rotateMiniMap"] = MapRenderState::rotateMiniMap;
        j["uiTextScale"] = MapRenderState::uiTextScale;
        j["miniMapScale"] = MapRenderState::miniMapScale;
        j["miniMapOffsetX"] = MapRenderState::miniMapOffsetX;
        j["miniMapOffsetY"] = MapRenderState::miniMapOffsetY;
        j["showWaypointsOnMinimap"] = MapRenderState::showWaypointsOnMinimap;
        j["showRadar"] = MapRenderState::showRadar;
        j["bigMapShowEntities"] = MapRenderState::bigMapShowEntities;
        j["bigMapShowMarkers"] = MapRenderState::bigMapShowMarkers;
        j["showChunkGrid"] = MapRenderState::showChunkGrid;
        j["caveModeType"] = MapRenderState::g_caveModeType;
        j["caveTopYAuto"] = MapRenderState::g_caveTopYAuto;
        j["caveTopY"] = MapRenderState::g_caveTopY;
        j["caveDepth"] = MapRenderState::g_caveDepth;
        j["legibleCaveMaps"] = MapRenderState::g_legibleCaveMaps;
        j["exportForceFullMap"] = MapRenderState::exportForceFullMap;
        j["exportMultipleImages"] = MapRenderState::exportMultipleImages;
        j["exportOpenFolder"] = MapRenderState::exportOpenFolder;
        j["exportScaleDownSquare"] = MapRenderState::exportScaleDownSquare;

        const auto seedMapSettings = SeedMapManager::GetSettings();
        json seedMapLayers = json::array();
        for (bool enabled : seedMapSettings.enabledLayers) seedMapLayers.push_back(enabled);
        j["seedMap"] = {
            {"enabled", seedMapSettings.enabled},
            {"searchMode", seedMapSettings.searchMode == SeedMapManager::SearchMode::Manual ? 1 : 0},
            {"dimension", static_cast<int>(seedMapSettings.targetDimension)},
            {"manualCenterX", seedMapSettings.manualCenterX},
            {"manualCenterZ", seedMapSettings.manualCenterZ},
            {"manualRadius", seedMapSettings.manualRadius},
            {"enabledLayers", std::move(seedMapLayers)}
        };

        json manualSeedsJson = json::object();
        for (const auto& [wId, sBits] : SeedMapManager::GetSavedManualSeeds()) {
            manualSeedsJson[wId] = std::to_string(sBits);
        }
        j["manualSeeds"] = manualSeedsJson;

        // 保存快捷键绑定 (持久化保存，存储按键及修饰键组合)
        auto saveHk = [&](const char* name, const MapRenderState::Hotkey& h) {
            json item;
            item["key"] = h.key;
            item["modifiers"] = h.modifiers;
            if (h.modifiers & MapRenderState::Hotkey::HK_MOD_CTRL)  item["ctrl"] = true;
            if (h.modifiers & MapRenderState::Hotkey::HK_MOD_SHIFT) item["shift"] = true;
            if (h.modifiers & MapRenderState::Hotkey::HK_MOD_ALT)   item["alt"] = true;
            j["hotkeys"][name] = item;
        };

        saveHk("openBigMap", MapRenderState::g_hotkeys.openBigMap);
        saveHk("openWaypointMgr", MapRenderState::g_hotkeys.openWaypointMgr);
        saveHk("openDeathPointMgr", MapRenderState::g_hotkeys.openDeathPointMgr);
        saveHk("toggleMinimap", MapRenderState::g_hotkeys.toggleMinimap);
        saveHk("toggleMinimapShape", MapRenderState::g_hotkeys.toggleMinimapShape);
        saveHk("toggleMinimapRot", MapRenderState::g_hotkeys.toggleMinimapRot);
        saveHk("holdEntities", MapRenderState::g_hotkeys.holdEntities);
        saveHk("toggleSeedMap", MapRenderState::g_hotkeys.toggleSeedMap);

        std::ofstream out(filePath);
        if (out.is_open()) {
            out << j.dump(4);
            out.close();
        }
    }

    static std::string GetBuiltinExtraText(const std::string& key, const std::string& langCode) {
        static const std::unordered_map<std::string, std::string> en = {
            {"SEED_MAP_TITLE", "Seed Map"},
            {"SEED_MAP_OPEN", "Open Seed Map"},
            {"SEED_MAP_AUTO_SEED", "Automatic seed"},
            {"SEED_MAP_RAW_SEED", "Raw client seed"},
            {"SEED_MAP_CAPTURED", "Captured from current world"},
            {"SEED_MAP_UNAVAILABLE", "Seed unavailable"},
            {"SEED_MAP_MANUAL_SEED", "Custom Seed"},
            {"SEED_MAP_APPLY_SEED", "Apply"},
            {"SEED_MAP_RESET_AUTO", "Auto"},
            {"SEED_MAP_INPUT_HINT", "Enter seed number or text..."},
            {"SEED_MAP_CUSTOM_ACTIVE", "Custom seed active"},
            {"SEED_MAP_PROFILE", "Bedrock 26.51"},
            {"SEED_MAP_DIMENSION", "Dimension"},
            {"SEED_MAP_VISIBLE", "Visible map"},
            {"SEED_MAP_MANUAL", "Manual search"},
            {"SEED_MAP_LAYERS", "Layers"},
            {"SEED_MAP_VERIFIED", "Fixture verified"},
            {"SEED_MAP_CUBIOMES_REFERENCE", "Cubiomes 26.51 prediction"},
            {"SEED_MAP_AWAITING_FIXTURE", "Awaiting fixture"},
            {"SEED_MAP_UNAVAILABLE_LAYER", "Unavailable"},
            {"SEED_MAP_SEARCH_X", "Center X"},
            {"SEED_MAP_SEARCH_Z", "Center Z"},
            {"SEED_MAP_RADIUS", "Radius"},
            {"SEED_MAP_SEARCH", "Search"},
            {"SEED_MAP_PROGRESS", "Progress %zu / %zu tiles"},
            {"SEED_MAP_RESULTS", "Results"},
            {"SEED_MAP_CENTER", "Center map"},
            {"SEED_MAP_SAVE_WAYPOINT", "Save waypoint"},
            {"SEED_MAP_CANDIDATE", "Fixture-verified seed location"},
            {"SEED_MAP_NO_RESULTS", "No verified markers in this area."},
            {"SEED_MAP_DIMENSION_MISMATCH", "Select the current dimension to draw markers on this map."},
            {"SEED_MAP_WORKING", "Calculating unloaded regions locally"},
            {"SEED_MAP_DISABLED", "Seed Map disabled"},
            {"SEED_MAP_CLEAR_ALL", "Clear all layers"},
            {"SEED_MAP_LEGEND", "Active map icons"},
            {"SEED_MAP_FIXTURE_LOCATION", "Fixture-verified seed location"},
            {"SEED_MAP_SEED_PREDICTION", "Cubiomes seed prediction"},
            {"SEED_MAP_UNKNOWN_LAYER", "Unknown layer"},
            {"SEED_MAP_LAYER_VILLAGE", "Village"},
            {"SEED_MAP_LAYER_PILLAGER_OUTPOST", "Pillager Outpost"},
            {"SEED_MAP_LAYER_DESERT_PYRAMID", "Desert Pyramid"},
            {"SEED_MAP_LAYER_JUNGLE_TEMPLE", "Jungle Temple"},
            {"SEED_MAP_LAYER_SWAMP_HUT", "Swamp Hut"},
            {"SEED_MAP_LAYER_IGLOO", "Igloo"},
            {"SEED_MAP_LAYER_WOODLAND_MANSION", "Woodland Mansion"},
            {"SEED_MAP_LAYER_OCEAN_MONUMENT", "Ocean Monument"},
            {"SEED_MAP_LAYER_OCEAN_RUIN", "Ocean Ruin"},
            {"SEED_MAP_LAYER_SHIPWRECK", "Shipwreck"},
            {"SEED_MAP_LAYER_BURIED_TREASURE", "Buried Treasure"},
            {"SEED_MAP_LAYER_RUINED_PORTAL", "Ruined Portal"},
            {"SEED_MAP_LAYER_RUINED_PORTAL_NETHER", "Ruined Portal (Nether)"},
            {"SEED_MAP_LAYER_STRONGHOLD", "Stronghold"},
            {"SEED_MAP_LAYER_MINESHAFT", "Mineshaft"},
            {"SEED_MAP_LAYER_ANCIENT_CITY", "Ancient City"},
            {"SEED_MAP_LAYER_TRAIL_RUINS", "Trail Ruins"},
            {"SEED_MAP_LAYER_TRIAL_CHAMBERS", "Trial Chambers"},
            {"SEED_MAP_LAYER_NETHER_FORTRESS", "Nether Fortress"},
            {"SEED_MAP_LAYER_BASTION_REMNANT", "Bastion Remnant"},
            {"SEED_MAP_LAYER_NETHER_FOSSIL", "Nether Fossil"},
            {"SEED_MAP_LAYER_END_CITY", "End City"},
            {"SEED_MAP_LAYER_END_GATEWAY", "End Gateway"},
            {"SEED_MAP_LAYER_OVERWORLD_BIOMES", "Overworld Biomes"},
            {"SEED_MAP_LAYER_NETHER_BIOMES", "Nether Biomes"},
            {"SEED_MAP_LAYER_END_BIOMES", "End Biomes"},
            {"SEED_MAP_LAYER_SLIME_CHUNKS", "Slime Chunks"},
            {"SEED_MAP_LAYER_OVERWORLD_ORES", "Overworld Ores"},
            {"SEED_MAP_LAYER_NETHER_ORES", "Nether Ores"},
            {"SEED_MAP_LAYER_UNDERGROUND_FEATURES", "Underground Features"},
            {"SEED_MAP_LAYER_DUNGEON_SPAWNER", "Dungeon / Spawner"},
            {"SEED_MAP_LAYER_WORLD_SPAWN", "World Spawn"},
            {"SEED_MAP_LAYER_MUSHROOM_FIELDS", "Mushroom Fields"},
            {"SEED_MAP_LAYER_CHERRY_GROVE", "Cherry Grove"},
            {"SEED_MAP_LAYER_BADLANDS", "Badlands"},
            {"SEED_MAP_LAYER_ICE_SPIKES", "Ice Spikes"},
            {"SEED_MAP_LAYER_MANGROVE_SWAMP", "Mangrove Swamp"},
            {"SEED_MAP_LAYER_PALE_GARDEN", "Pale Garden"},
            {"SEED_MAP_LAYER_SOUL_SAND_VALLEY", "Soul Sand Valley"},
            {"SEED_MAP_LAYER_CRIMSON_FOREST", "Crimson Forest"},
            {"SEED_MAP_LAYER_WARPED_FOREST", "Warped Forest"},
            {"SEED_MAP_LAYER_BASALT_DELTAS", "Basalt Deltas"},
            {"SEED_MAP_LAYER_END_SMALL_ISLANDS", "Small End Islands"},
            {"SEED_MAP_LAYER_END_MIDLANDS", "End Midlands"},
            {"SEED_MAP_LAYER_END_HIGHLANDS", "End Highlands"},
            {"SEED_MAP_LAYER_END_BARRENS", "End Barrens"},
            {"SEED_MAP_LAYER_ABANDONED_CAMP", "Abandoned Camp"},
            {"SEED_MAP_LAYER_SULFUR_CAVES", "Sulfur Caves"},
            {"SEED_MAP_LAYER_DAPPLED_FOREST", "Dappled Forest"},
            {"DIM_OVERWORLD", "Overworld"},
            {"DIM_NETHER", "Nether"},
            {"DIM_END", "The End"},
            {"DIM_UNKNOWN", "Unknown Dimension"},
            {"MODERN_DEATH_MANAGER", "Death Records"},
            {"DEATH_POINTS_TITLE", "Death Records (Press 'I' or 'Esc' to Close)##Deaths"},
            {"DEATH_POINTS_EMPTY", "No death records yet."},
            {"DEATH_POINTS_HINT", "Death records are saved per world. Teleportation is only available within the current dimension."},
            {"DEATH_POINT_TELEPORT", "Teleport"},
            {"DEATH_POINT_DELETE", "Delete"},
            {"DEATH_POINT_CREATE_WP", "Create Waypoint"},
            {"DEATH_POINT_WP_PREFIX", "Death"},
            {"MODERN_DIMENSION_MISMATCH", "Different dimension"},
            {"OPEN_DEATH_MANAGER", "Open Death Records"},
            {"DEATH_POINT_ALREADY_CONVERTED", "Converted"}
        };
        static const std::unordered_map<std::string, std::string> zhCN = {
            {"SEED_MAP_TITLE", "种子地图"},
            {"SEED_MAP_OPEN", "打开种子地图"},
            {"SEED_MAP_AUTO_SEED", "自动读取种子"},
            {"SEED_MAP_RAW_SEED", "客户端原始种子"},
            {"SEED_MAP_CAPTURED", "已从当前世界读取"},
            {"SEED_MAP_UNAVAILABLE", "无法读取种子"},
            {"SEED_MAP_MANUAL_SEED", "自定义种子"},
            {"SEED_MAP_APPLY_SEED", "应用"},
            {"SEED_MAP_RESET_AUTO", "恢复自动"},
            {"SEED_MAP_INPUT_HINT", "输入种子数值或文字..."},
            {"SEED_MAP_CUSTOM_ACTIVE", "已使用自定义种子"},
            {"SEED_MAP_PROFILE", "基岩版 26.51"},
            {"SEED_MAP_DIMENSION", "维度"},
            {"SEED_MAP_VISIBLE", "可见地图"},
            {"SEED_MAP_MANUAL", "手动搜索"},
            {"SEED_MAP_LAYERS", "图层"},
            {"SEED_MAP_VERIFIED", "已由样本验证"},
            {"SEED_MAP_CUBIOMES_REFERENCE", "Cubiomes 26.51 种子预测"},
            {"SEED_MAP_AWAITING_FIXTURE", "等待样本验证"},
            {"SEED_MAP_UNAVAILABLE_LAYER", "不可用"},
            {"SEED_MAP_SEARCH_X", "中心 X"},
            {"SEED_MAP_SEARCH_Z", "中心 Z"},
            {"SEED_MAP_RADIUS", "半径"},
            {"SEED_MAP_SEARCH", "搜索"},
            {"SEED_MAP_PROGRESS", "进度 %zu / %zu 个瓦片"},
            {"SEED_MAP_RESULTS", "结果"},
            {"SEED_MAP_CENTER", "居中地图"},
            {"SEED_MAP_SAVE_WAYPOINT", "保存地标"},
            {"SEED_MAP_CANDIDATE", "已由样本验证的位置候选"},
            {"SEED_MAP_NO_RESULTS", "此区域没有已验证的标记。"},
            {"SEED_MAP_DIMENSION_MISMATCH", "请选择当前维度以在此地图上绘制标记。"},
            {"SEED_MAP_WORKING", "正在本地计算未加载区域"},
            {"SEED_MAP_DISABLED", "种子地图已关闭"},
            {"SEED_MAP_CLEAR_ALL", "取消全部勾选"},
            {"SEED_MAP_LEGEND", "当前图标"},
            {"SEED_MAP_FIXTURE_LOCATION", "已由样本验证的种子位置"},
            {"SEED_MAP_SEED_PREDICTION", "Cubiomes 种子预测"},
            {"SEED_MAP_UNKNOWN_LAYER", "未知图层"},
            {"SEED_MAP_LAYER_VILLAGE", "村庄"},
            {"SEED_MAP_LAYER_PILLAGER_OUTPOST", "掠夺者前哨站"},
            {"SEED_MAP_LAYER_DESERT_PYRAMID", "沙漠神殿"},
            {"SEED_MAP_LAYER_JUNGLE_TEMPLE", "丛林神庙"},
            {"SEED_MAP_LAYER_SWAMP_HUT", "沼泽小屋"},
            {"SEED_MAP_LAYER_IGLOO", "雪屋"},
            {"SEED_MAP_LAYER_WOODLAND_MANSION", "林地府邸"},
            {"SEED_MAP_LAYER_OCEAN_MONUMENT", "海底神殿"},
            {"SEED_MAP_LAYER_OCEAN_RUIN", "海洋废墟"},
            {"SEED_MAP_LAYER_SHIPWRECK", "沉船"},
            {"SEED_MAP_LAYER_BURIED_TREASURE", "埋藏宝藏"},
            {"SEED_MAP_LAYER_RUINED_PORTAL", "废弃传送门"},
            {"SEED_MAP_LAYER_RUINED_PORTAL_NETHER", "下界废弃传送门"},
            {"SEED_MAP_LAYER_STRONGHOLD", "要塞"},
            {"SEED_MAP_LAYER_MINESHAFT", "废弃矿井"},
            {"SEED_MAP_LAYER_ANCIENT_CITY", "远古城市"},
            {"SEED_MAP_LAYER_TRAIL_RUINS", "古迹废墟"},
            {"SEED_MAP_LAYER_TRIAL_CHAMBERS", "试炼密室"},
            {"SEED_MAP_LAYER_NETHER_FORTRESS", "下界要塞"},
            {"SEED_MAP_LAYER_BASTION_REMNANT", "堡垒遗迹"},
            {"SEED_MAP_LAYER_NETHER_FOSSIL", "下界化石"},
            {"SEED_MAP_LAYER_END_CITY", "末地城"},
            {"SEED_MAP_LAYER_END_GATEWAY", "末地折跃门"},
            {"SEED_MAP_LAYER_OVERWORLD_BIOMES", "主世界群系"},
            {"SEED_MAP_LAYER_NETHER_BIOMES", "下界群系"},
            {"SEED_MAP_LAYER_END_BIOMES", "末地群系"},
            {"SEED_MAP_LAYER_SLIME_CHUNKS", "史莱姆区块"},
            {"SEED_MAP_LAYER_OVERWORLD_ORES", "主世界矿石"},
            {"SEED_MAP_LAYER_NETHER_ORES", "下界矿石"},
            {"SEED_MAP_LAYER_UNDERGROUND_FEATURES", "地下特征"},
            {"SEED_MAP_LAYER_DUNGEON_SPAWNER", "地牢 / 刷怪笼"},
            {"SEED_MAP_LAYER_WORLD_SPAWN", "世界出生点"},
            {"SEED_MAP_LAYER_MUSHROOM_FIELDS", "蘑菇岛"},
            {"SEED_MAP_LAYER_CHERRY_GROVE", "樱花林"},
            {"SEED_MAP_LAYER_BADLANDS", "恶地"},
            {"SEED_MAP_LAYER_ICE_SPIKES", "冰刺"},
            {"SEED_MAP_LAYER_MANGROVE_SWAMP", "红树林沼泽"},
            {"SEED_MAP_LAYER_PALE_GARDEN", "苍白花园"},
            {"SEED_MAP_LAYER_SOUL_SAND_VALLEY", "灵魂沙峡谷"},
            {"SEED_MAP_LAYER_CRIMSON_FOREST", "绯红森林"},
            {"SEED_MAP_LAYER_WARPED_FOREST", "诡异森林"},
            {"SEED_MAP_LAYER_BASALT_DELTAS", "玄武岩三角洲"},
            {"SEED_MAP_LAYER_END_SMALL_ISLANDS", "末地小型岛屿"},
            {"SEED_MAP_LAYER_END_MIDLANDS", "末地中部地带"},
            {"SEED_MAP_LAYER_END_HIGHLANDS", "末地高地"},
            {"SEED_MAP_LAYER_END_BARRENS", "末地荒地"},
            {"SEED_MAP_LAYER_ABANDONED_CAMP", "废弃营地"},
            {"SEED_MAP_LAYER_SULFUR_CAVES", "硫磺洞穴"},
            {"SEED_MAP_LAYER_DAPPLED_FOREST", "斑驳森林"},
            {"DIM_OVERWORLD", "主世界"},
            {"DIM_NETHER", "下界"},
            {"DIM_END", "末地"},
            {"DIM_UNKNOWN", "未知维度"},
            {"MODERN_DEATH_MANAGER", "死亡记录"},
            {"DEATH_POINTS_TITLE", "死亡记录 (按 'I' 或 'Esc' 关闭)##Deaths"},
            {"DEATH_POINTS_EMPTY", "暂无死亡记录。"},
            {"DEATH_POINTS_HINT", "死亡记录按世界保存，只能传送当前维度的记录。"},
            {"DEATH_POINT_TELEPORT", "传送"},
            {"DEATH_POINT_DELETE", "删除"},
            {"DEATH_POINT_CREATE_WP", "转为路径点"},
            {"DEATH_POINT_WP_PREFIX", "死亡点"},
            {"MODERN_DIMENSION_MISMATCH", "不在当前维度"},
            {"OPEN_DEATH_MANAGER", "打开死亡记录"},
            {"DEATH_POINT_ALREADY_CONVERTED", "已转为路径点"}
        };
        static const std::unordered_map<std::string, std::string> zhTW = {
            {"SEED_MAP_TITLE", "種子地圖"},
            {"SEED_MAP_OPEN", "開啟種子地圖"},
            {"SEED_MAP_AUTO_SEED", "自動讀取種子"},
            {"SEED_MAP_RAW_SEED", "客戶端原始種子"},
            {"SEED_MAP_CAPTURED", "已從目前世界讀取"},
            {"SEED_MAP_UNAVAILABLE", "無法讀取種子"},
            {"SEED_MAP_MANUAL_SEED", "自訂種子"},
            {"SEED_MAP_APPLY_SEED", "套用"},
            {"SEED_MAP_RESET_AUTO", "恢復自動"},
            {"SEED_MAP_INPUT_HINT", "輸入種子數值或文字..."},
            {"SEED_MAP_CUSTOM_ACTIVE", "已使用自訂種子"},
            {"SEED_MAP_PROFILE", "基岩版 26.51"},
            {"SEED_MAP_DIMENSION", "維度"},
            {"SEED_MAP_VISIBLE", "可見地圖"},
            {"SEED_MAP_MANUAL", "手動搜尋"},
            {"SEED_MAP_LAYERS", "圖層"},
            {"SEED_MAP_VERIFIED", "已由樣本驗證"},
            {"SEED_MAP_CUBIOMES_REFERENCE", "Cubiomes 26.51 種子預測"},
            {"SEED_MAP_AWAITING_FIXTURE", "等待樣本驗證"},
            {"SEED_MAP_UNAVAILABLE_LAYER", "不可用"},
            {"SEED_MAP_SEARCH_X", "中心 X"},
            {"SEED_MAP_SEARCH_Z", "中心 Z"},
            {"SEED_MAP_RADIUS", "半徑"},
            {"SEED_MAP_SEARCH", "搜尋"},
            {"SEED_MAP_PROGRESS", "進度 %zu / %zu 個瓦片"},
            {"SEED_MAP_RESULTS", "結果"},
            {"SEED_MAP_CENTER", "置中地圖"},
            {"SEED_MAP_SAVE_WAYPOINT", "儲存地標"},
            {"SEED_MAP_CANDIDATE", "已由樣本驗證的位置候選"},
            {"SEED_MAP_NO_RESULTS", "此區域沒有已驗證的標記。"},
            {"SEED_MAP_DIMENSION_MISMATCH", "請選擇目前維度以在此地圖上繪製標記。"},
            {"SEED_MAP_WORKING", "正在本地計算未載入區域"},
            {"SEED_MAP_DISABLED", "種子地圖已關閉"},
            {"SEED_MAP_CLEAR_ALL", "取消全部勾選"},
            {"SEED_MAP_LEGEND", "目前圖示"},
            {"SEED_MAP_FIXTURE_LOCATION", "已由樣本驗證的種子位置"},
            {"SEED_MAP_SEED_PREDICTION", "Cubiomes 種子預測"},
            {"SEED_MAP_UNKNOWN_LAYER", "未知圖層"},
            {"SEED_MAP_LAYER_VILLAGE", "村莊"},
            {"SEED_MAP_LAYER_PILLAGER_OUTPOST", "掠奪者前哨站"},
            {"SEED_MAP_LAYER_DESERT_PYRAMID", "沙漠神殿"},
            {"SEED_MAP_LAYER_JUNGLE_TEMPLE", "叢林神廟"},
            {"SEED_MAP_LAYER_SWAMP_HUT", "沼澤小屋"},
            {"SEED_MAP_LAYER_IGLOO", "雪屋"},
            {"SEED_MAP_LAYER_WOODLAND_MANSION", "林地府邸"},
            {"SEED_MAP_LAYER_OCEAN_MONUMENT", "海底神殿"},
            {"SEED_MAP_LAYER_OCEAN_RUIN", "海洋廢墟"},
            {"SEED_MAP_LAYER_SHIPWRECK", "沉船"},
            {"SEED_MAP_LAYER_BURIED_TREASURE", "埋藏寶藏"},
            {"SEED_MAP_LAYER_RUINED_PORTAL", "廢棄傳送門"},
            {"SEED_MAP_LAYER_RUINED_PORTAL_NETHER", "地獄廢棄傳送門"},
            {"SEED_MAP_LAYER_STRONGHOLD", "要塞"},
            {"SEED_MAP_LAYER_MINESHAFT", "廢棄礦井"},
            {"SEED_MAP_LAYER_ANCIENT_CITY", "遠古城市"},
            {"SEED_MAP_LAYER_TRAIL_RUINS", "古蹟廢墟"},
            {"SEED_MAP_LAYER_TRIAL_CHAMBERS", "試煉密室"},
            {"SEED_MAP_LAYER_NETHER_FORTRESS", "地獄要塞"},
            {"SEED_MAP_LAYER_BASTION_REMNANT", "堡壘遺蹟"},
            {"SEED_MAP_LAYER_NETHER_FOSSIL", "地獄化石"},
            {"SEED_MAP_LAYER_END_CITY", "終界城"},
            {"SEED_MAP_LAYER_END_GATEWAY", "終界折躍門"},
            {"SEED_MAP_LAYER_OVERWORLD_BIOMES", "主世界生態域"},
            {"SEED_MAP_LAYER_NETHER_BIOMES", "地獄生態域"},
            {"SEED_MAP_LAYER_END_BIOMES", "終界生態域"},
            {"SEED_MAP_LAYER_SLIME_CHUNKS", "史萊姆區塊"},
            {"SEED_MAP_LAYER_OVERWORLD_ORES", "主世界礦石"},
            {"SEED_MAP_LAYER_NETHER_ORES", "地獄礦石"},
            {"SEED_MAP_LAYER_UNDERGROUND_FEATURES", "地下特徵"},
            {"SEED_MAP_LAYER_DUNGEON_SPAWNER", "地牢 / 生怪磚"},
            {"SEED_MAP_LAYER_WORLD_SPAWN", "世界出生點"},
            {"SEED_MAP_LAYER_MUSHROOM_FIELDS", "蘑菇島"},
            {"SEED_MAP_LAYER_CHERRY_GROVE", "櫻花林"},
            {"SEED_MAP_LAYER_BADLANDS", "惡地"},
            {"SEED_MAP_LAYER_ICE_SPIKES", "冰刺"},
            {"SEED_MAP_LAYER_MANGROVE_SWAMP", "紅樹林沼澤"},
            {"SEED_MAP_LAYER_PALE_GARDEN", "蒼白花園"},
            {"SEED_MAP_LAYER_SOUL_SAND_VALLEY", "靈魂砂峽谷"},
            {"SEED_MAP_LAYER_CRIMSON_FOREST", "緋紅森林"},
            {"SEED_MAP_LAYER_WARPED_FOREST", "扭曲森林"},
            {"SEED_MAP_LAYER_BASALT_DELTAS", "玄武岩三角洲"},
            {"SEED_MAP_LAYER_END_SMALL_ISLANDS", "終界小型島嶼"},
            {"SEED_MAP_LAYER_END_MIDLANDS", "終界中部地帶"},
            {"SEED_MAP_LAYER_END_HIGHLANDS", "終界高地"},
            {"SEED_MAP_LAYER_END_BARRENS", "終界荒地"},
            {"SEED_MAP_LAYER_ABANDONED_CAMP", "廢棄營地"},
            {"SEED_MAP_LAYER_SULFUR_CAVES", "硫磺洞穴"},
            {"SEED_MAP_LAYER_DAPPLED_FOREST", "斑駁森林"},
            {"DIM_OVERWORLD", "主世界"},
            {"DIM_NETHER", "下界"},
            {"DIM_END", "終界"},
            {"DIM_UNKNOWN", "未知維度"},
            {"MODERN_DEATH_MANAGER", "死亡紀錄"},
            {"DEATH_POINTS_TITLE", "死亡紀錄 (按 'I' 或 'Esc' 關閉)##Deaths"},
            {"DEATH_POINTS_EMPTY", "暫無死亡紀錄。"},
            {"DEATH_POINTS_HINT", "死亡紀錄按世界保存，只能傳送目前維度的紀錄。"},
            {"DEATH_POINT_TELEPORT", "傳送"},
            {"DEATH_POINT_DELETE", "刪除"},
            {"DEATH_POINT_CREATE_WP", "轉為路徑點"},
            {"DEATH_POINT_WP_PREFIX", "死亡點"},
            {"MODERN_DIMENSION_MISMATCH", "不在目前維度"},
            {"OPEN_DEATH_MANAGER", "開啟死亡紀錄"},
            {"DEATH_POINT_ALREADY_CONVERTED", "已轉為路徑點"}
        };

        const auto* table = &en;
        if (langCode == "zh_CN") table = &zhCN;
        else if (langCode == "zh_TW") table = &zhTW;
        auto it = table->find(key);
        if (it != table->end()) return it->second;
        auto fallback = en.find(key);
        return fallback != en.end() ? fallback->second : std::string();
    }

    const char* GetText(const std::string& key) {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        auto it = g_translationCache.find(key);
        if (it != g_translationCache.end()) {
            return it->second.c_str();
        }

        std::string_view sv = ll::i18n::getInstance().get(key, g_currentLanguage);
        if (sv == key) {
            // 1. 优先尝试直接从磁盘对应语言的 JSON 读取 (无需重启即可生效)
            try {
                auto p = g_langDir / (g_currentLanguage + ".json");
                if (std::filesystem::exists(p)) {
                    std::ifstream ifs(p);
                    if (ifs.is_open()) {
                        json j;
                        ifs >> j;
                        if (j.contains(key) && j[key].is_string()) {
                            std::string val = j[key].get<std::string>();
                            ll::i18n::getInstance().set(g_currentLanguage, key, val);
                            g_translationCache[key] = val;
                            return g_translationCache[key].c_str();
                        }
                    }
                }
            } catch (...) {}

            // 2. 尝试从内置 16 种多语言兜底字典读取
            auto fit = g_builtinTranslations.find(key);
            if (fit != g_builtinTranslations.end()) {
                auto langIt = fit->second.find(g_currentLanguage);
                if (langIt != fit->second.end()) {
                    g_translationCache[key] = langIt->second;
                    return g_translationCache[key].c_str();
                }
                auto enIt = fit->second.find("en_US");
                if (enIt != fit->second.end()) {
                    g_translationCache[key] = enIt->second;
                    return g_translationCache[key].c_str();
                }
            }

            // 3. 尝试从种子地图等额外字典读取
            std::string extra = GetBuiltinExtraText(key, g_currentLanguage);
            if (!extra.empty()) {
                g_translationCache[key] = extra;
                return g_translationCache[key].c_str();
            }
        }

        g_translationCache[key] = std::string(sv);
        return g_translationCache[key].c_str();
    }
}
