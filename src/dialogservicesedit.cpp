#include "dialogservicesedit.h"
#include "ui_dialogservicesedit.h"

#include <QPushButton>
#include <QMessageBox>

DialogServicesEdit::DialogServicesEdit(database *pdata, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DialogServicesEdit)
{
	ui->setupUi(this);

	m_servComm = pdata->m_customer->m_serviceComm;
	m_tax = pdata->getIsTax();

	//Affiche ou pas les taxes
	ui->comboBox_tax->setVisible(m_tax);
	if(m_tax) ui->label_tax->setText( tr("HT") );
	else    ui->label_tax->setText( tr("TTC") );
	ui->label_titletax->setVisible(m_tax);

	ui->dateTimeEdit->setDateTime( QDateTime::currentDateTime());
	listInterCommToTable();
	//Event
	ui->pushButton_add->setEnabled(false);
	ui->pushButton_edit->setEnabled(false);
	connect(ui->lineEdit_Name, SIGNAL(textChanged(const QString &)), this, SLOT(checkConditions()));
}

DialogServicesEdit::~DialogServicesEdit()
{
	delete ui;
}

/**
	 Test les conditions pour activer le bouton QDialogButtonBox::Ok
  */
void DialogServicesEdit::checkConditions()
{
	if(!ui->lineEdit_Name->text().isEmpty()){
		ui->pushButton_add->setEnabled(true);
		ui->pushButton_edit->setEnabled(true);
	}
	else{
		ui->pushButton_add->setEnabled(false);
		ui->pushButton_edit->setEnabled(false);
	}
}


/**
	Renseigne le titre
	@param titre de la fenetre
  */
void DialogServicesEdit::setTitle(QString val){
	ui->groupBox_IntCom->setTitle(val);
}


/**
  Renseigne les informations du service dans les champs
  */
void DialogServicesEdit::loadValuesFormService(){
	//Charge les champs
	ui->lineEdit_Name->setText( m_servComm->getName() );
	ui->dateTimeEdit->setDateTime( m_servComm->getDate() );
	ui->doubleSpinBox->setValue( m_servComm->getPrice() );
	ui->textEdit_Desc->setPlainText( m_servComm->getDescription() );
}


/**
	efface les informations de l intervention des champs
  */
void DialogServicesEdit::loadEmptyValues() {
	ui->lineEdit_Name->setText("");
	ui->dateTimeEdit->setDateTime( QDateTime::currentDateTime() );
	ui->doubleSpinBox->setValue( 0 );
	ui->textEdit_Desc->setPlainText( "" );
}

/**
  charge les informations du service dans lobjet
  */
void DialogServicesEdit::setValuesToService(){
	//Charge les champs
	//resize
	QString desc = ui->textEdit_Desc->toPlainText();
	if(desc.size() > 1000) desc.resize(1000);
	m_servComm->setName( ui->lineEdit_Name->text()  );
	m_servComm->setDate( ui->dateTimeEdit->dateTime() );
	m_servComm->setPrice( ui->doubleSpinBox->value() );
	m_servComm->setDescription( desc );
}

void DialogServicesEdit::on_buttonBox_accepted()
{
//
}


/**
	Affiche les Services generales
*/
void DialogServicesEdit::listInterCommToTable()
{
	service_common::serviceCommList ilist;

	//Clear les items, attention tjs utiliser la fonction clear()
	ui->tableWidget_Inter->clear();
	for (int i=ui->tableWidget_Inter->rowCount()-1; i >= 0; --i)
		ui->tableWidget_Inter->removeRow(i);
	for (int i=ui->tableWidget_Inter->columnCount()-1; i>=0; --i)
		ui->tableWidget_Inter->removeColumn(i);


	ui->tableWidget_Inter->setSortingEnabled(false);
	//Style de la table de l intervention
	ui->tableWidget_Inter->setColumnCount(2);
	ui->tableWidget_Inter->setColumnWidth(1,225);
	ui->tableWidget_Inter->setColumnWidth(2,100);
	ui->tableWidget_Inter->setColumnHidden(0, true); //On cache la colonne des ID
	ui->tableWidget_Inter->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidget_Inter->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->tableWidget_Inter->setEditTriggers(QAbstractItemView::NoEditTriggers);
	QStringList titles;
	titles << tr("Id") << tr("Nom");
	ui->tableWidget_Inter->setHorizontalHeaderLabels( titles );

   //Recuperation des services commun avec ID = 0
	m_servComm->getServiceCommList(ilist, "NAME", "", "");

	// list all customers
	for(unsigned int i=0; i<ilist.id.size(); i++){
		QTableWidgetItem *item_ID      = new QTableWidgetItem();
		QTableWidgetItem *item_NAME     = new QTableWidgetItem();

		item_ID->setData(Qt::DisplayRole, QString::number(ilist.id.at(i)));
		item_NAME->setData(Qt::DisplayRole, ilist.name.at(i));

		//definir le tableau
		ui->tableWidget_Inter->setRowCount(i+1);

		//remplir les champs
		ui->tableWidget_Inter->setItem(i, 0, item_ID);
		ui->tableWidget_Inter->setItem(i, 1, item_NAME);
	}
	ui->tableWidget_Inter->setSortingEnabled(true);

	//Si pas de produit on desactive les boutons
	if( ui->tableWidget_Inter->rowCount() <=0 ) {
		ui->pushButton_edit->setEnabled(false);
		ui->pushButton_del->setEnabled(false);
	}else{
		ui->pushButton_edit->setEnabled(true);
		ui->pushButton_del->setEnabled(true);
	}
}

void DialogServicesEdit::on_tableWidget_Inter_itemSelectionChanged()
{
	int m_index = ui->tableWidget_Inter->currentRow();
	m_servComm->loadFromID( ui->tableWidget_Inter->item(m_index, 0)->text().toInt() );
	//Charge les champs
	loadValuesFormService();
}
/**
	Ajoute un intervention generale
  */
void DialogServicesEdit::on_pushButton_add_clicked()
{
	if(m_servComm->isHere( ui->lineEdit_Name->text() )){
		QMessageBox::warning(this, tr("Attention"), tr("Service d&eacute;ja pr&eacute;sent...<br>Merci de changer de nom") );
	}
	else{
		setValuesToService();
		if( m_servComm->create() ) listInterCommToTable();
		loadEmptyValues();
	}
}

/**
	  Supprime un intervention generale
  */
void DialogServicesEdit::on_pushButton_del_clicked()
{
	int ret = QMessageBox::warning(this, tr("Attention"),
								   tr("Voulez-vous vraiment supprimer cette intervention:<br><b>")
								   + m_servComm->getName(),
								   QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);

	if(ret == QMessageBox::Yes){
		m_servComm->remove();
		//rafraichir la liste
		listInterCommToTable();
	}
}

/**
	Modification de l intervention generale
  */
void DialogServicesEdit::on_pushButton_edit_clicked()
{
	int id = m_servComm->isHere( ui->lineEdit_Name->text());
	if(  (id > 0) && (id != m_servComm->getId()) ){
		QMessageBox::warning(this, tr("Attention"), tr("Service d&eacute;ja pr&eacute;sent...<br>Merci de changer de nom") );
	}
	else{
		setValuesToService();
		if( m_servComm->update() ) listInterCommToTable();
	}
}


